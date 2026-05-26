/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <athrd.h>
#include <aline.h>
#include <alock.h>
#include <atimer.h>
#include <asortque.h>
#include <stdatomic.h>

/* timespec_get */
#if defined (__C_POSIX__)
__weak __unused int timespec_get(struct timespec *ts, int base){
	if(ts == NULL || base != TIME_UTC){
		return 0;
	}
	if(clock_gettime(CLOCK_REALTIME, ts) == 0){
		return base;
	}else{
		return 0;
	}
}
#elif defined (__C_WINDOWS__)
__weak  __unused int timespec_get(struct timespec *ts, int base){
    if (ts == NULL || base != TIME_UTC)
        return 0;

    FILETIME ft;
    /* 优先使用更高精度的版本（Win8+），否则回退 */
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32) {
        FARPROC proc = GetProcAddress(hKernel32, "GetSystemTimePreciseAsFileTime");
        if (proc) {
            ((void (WINAPI*)(FILETIME*))proc)(&ft);
        } else {
            GetSystemTimeAsFileTime(&ft);
        }
    } else {
        GetSystemTimeAsFileTime(&ft);
    }

    /* FILETIME 到 64 位 100 纳秒数 */
    ULARGE_INTEGER uli;
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    /* 1601-01-01 到 1970-01-01 的 100 纳秒间隔数：116444736000000000 */
    const ULONGLONG EPOCH_DIFF = 116444736000000000ULL;
    uli.QuadPart -= EPOCH_DIFF;

    ts->tv_sec  = (time_t)(uli.QuadPart / 10000000ULL);   /* 100ns -> 秒 */
    ts->tv_nsec = (long)((uli.QuadPart % 10000000ULL) * 100); /* 余数 -> 纳秒 */

    return base;
}
#endif /* __C_POSIX__ || __C_WINDOWS__ */
/* timespec_get */



typedef struct{
    void(*call)(void* data);    //任务处理函数
    void*       data;           //任务数据
    int64_t     id;             //任务id
    uint32_t    num;            //执行次数, =-1时为长期任务
    uint32_t    cycle;          //执行周期,ms
    uint32_t    wait;           //等待时间,ms,=0时立即执行
    atomic_bool rm_flag;        //被删除
}AWork;
__weak int A_OBJ_CMPD(AWork)(const AWork* self, const AWork* that){
    int ret = A_CMPD(uint32_t, self->wait, that->wait);
    if(ret == 0) ret = A_CMPD(int64_t, self->id, that->id);
    return ret;
}
A_TYPE_REGISTER(AWork);
static inline void AWork_call(AWork* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(!atomic_load_explicit(&self->rm_flag, memory_order_relaxed)){
        self->call(self->data);
        self->wait = self->cycle;   //重置等待时间
        if(self->num != aLongWork){
            if(__a_likely(self->num != 0)){
                self->num--;
            }
        }
    }
}
static inline void AWork_set_rm(AWork* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    atomic_store_explicit(&self->rm_flag, true, memory_order_relaxed);
}
__unused static inline void AWork_clean_rm(AWork* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    atomic_store_explicit(&self->rm_flag, false, memory_order_relaxed);
}



ASortque_Define(AWork);
ASortque_Generate(AWork);
A_TYPE_REGISTER(ASortque(AWork));

typedef struct{
    ASortque(AWork) que;
}AWorkQue;
__weak void A_OBJ_INIT(AWorkQue)(AWorkQue* self){
    self->que = A_INIT(ASortque(AWork));
}
__weak void A_OBJ_DEST(AWorkQue)(AWorkQue* self){
    A_DEST(ASortque(AWork), self->que);
}
__weak void A_OBJ_COPY(AWorkQue)(AWorkQue* self, __unused const AWorkQue* that){
    self->que = A_COPY(ASortque(AWork), that->que);
}
__weak int A_OBJ_CMPD(AWorkQue)(const AWorkQue* self, const AWorkQue* that){
    return A_CMPD(ASortque(AWork), self->que, that->que);
}
A_TYPE_REGISTER(AWorkQue);

static inline void AWorkQue_rm(AWorkQue* self, int64_t id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(id < 0)){
        aExcSet(AEXC_outdomain);
        return;
    }

    auto que = &self->que;
    for(uint32_t i = 0; i < que->f->getNumber(que); i++){
        auto work = que->f->at(que, i);
        if(work != nullptr && work->id == id){
            que->f->rm(que, i);
            break;
        }
    }
}
static inline void AWorkQue_push(AWorkQue* self, AWork work){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto que = &self->que;
    que->f->ins(que, work);
}
static inline AWork AWorkQue_pop(AWorkQue* self){
    AWork work = {};
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return work;
    }

    auto que = &self->que;
    que->f->popMin(que, &work);
    return work;
}
static inline void AWorkQue_updataWait(AWorkQue* self, uint32_t time){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(time < 1)){
        return;
    }
    auto que = &self->que;
    forEach(it, *que){
        auto work = it.p;
        if(work != nullptr){
            if(work->wait > time){
                work->wait -= time;
            }else{
                work->wait = 0;
            }
        }
    }
}
static inline AWorkQue AWorkQue_getCallQue(AWorkQue* self){
    auto wq = A_INIT(AWorkQue);

    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return wq;
    }
    uint32_t n = self->que.f->getNumber(&self->que);
    for(uint32_t i = 0; i < n; i++){
        aExcClean();
        AWork work = AWorkQue_pop(self);
        if(aExcOccur()){
            break;
        }

        if(work.wait != 0){
            AWorkQue_push(self, work);
            break;
        }else{
            AWorkQue_push(&wq, work);
            if(aExcOccur()){
                break;
            }
        }
    }

    return wq;
}
static inline void AWorkQue_putQue(AWorkQue* self, AWorkQue* that){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    uint32_t n = that->que.f->getNumber(&that->que);
    for(uint32_t i = 0; i < n; i++){
        aExcClean();
        AWork work = AWorkQue_pop(that);
        if(aExcOccur()){
            break;
        }

        if(!work.rm_flag && work.num != 0){
            AWorkQue_push(self, work);
            if(aExcOccur()){
                break;
            }
        }
    }
}
static inline bool AWorkQue_empty(const AWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return true;
    }
    auto que = &self->que;
    return que->f->empty(que);
}
static inline uint32_t AWorkQue_next(const AWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return true;
    }
    auto que = &self->que;
    if(que->f->getNumber(que) != 0){
        auto p = que->f->at(que, 0);
        return p->wait;
    }
    return 0xffffff;
}
static inline void AWorkQue_call(const AWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto que = &self->que;
    forEach(it, *que){
        auto work = it.p;
        aExcClean();
        AWork_call(work);
        aExcClean();
    }
}



/* 毫秒级定时器 */
typedef struct{
    thrd_t      tid;
    AMtxCnd     lock;
    AClock      clock;
    AWorkQue    queue;
    AWorkQue    call_queue;
    bool stat;
    int thret;
}ATimer;
__weak void A_OBJ_INIT(ATimer)(ATimer* self){
    aExcClean();
    self->lock = A_INIT(AMtxCnd); if(aExcOccur()){ return; }
    self->clock = A_INIT(AClock); if(aExcOccur()){ return; }
    self->queue = A_INIT(AWorkQue); if(aExcOccur()){ return; }
    self->call_queue = A_INIT(AWorkQue); if(aExcOccur()){ return; }
    self->stat = false;
    self->thret = 0;
}
__weak void A_OBJ_DEST(ATimer)(ATimer* self){
    A_DEST(AClock, self->clock);
    A_DEST(AMtxCnd, self->lock);
    A_DEST(AWorkQue, self->queue);
    A_DEST(AWorkQue, self->call_queue);
    self->stat = false;
    self->thret = 0;
}
__weak void A_OBJ_COPY(ATimer)(ATimer* self, const ATimer* that){
    self->lock = A_INIT(AMtxCnd); if(aExcOccur()){ return; }
    self->clock = A_INIT(AClock); if(aExcOccur()){ return; }
    self->queue = A_COPY(AWorkQue, that->queue); if(aExcOccur()){ return; }
    self->call_queue = A_COPY(AWorkQue, that->call_queue); if(aExcOccur()){ return; }
    self->stat = false;
    self->thret = 0;
}
__weak int A_OBJ_CMPD(ATimer)(const ATimer* self, const ATimer* that){
    return  A_CMPD(AWorkQue, self->queue, that->queue);
}
A_TYPE_REGISTER(ATimer);

static inline void ATimer_main(ATimer* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    while(1){
        aExcClean();auto key = AMtxCnd_lock(&self->lock);if(aExcOccur()){
            return;
        }

        if(!self->stat){
            A_DEST(AAutoKey, key);
            return;
        }

        while(self->stat && AWorkQue_empty(&self->queue)){
            aExcClean(); AMtxCnd_wait(&self->lock); if(aExcOccur()){
                A_DEST(AAutoKey, key);
                return;
            }
        }

        AClock start = self->clock;
        AClock now = A_INIT(AClock);
        self->clock = now;

        AWorkQue_updataWait(&self->queue, (uint32_t)(AClock_usDiff(now, start) / 1000));
        if(aExcOccur()){ return; }

        self->call_queue = AWorkQue_getCallQue(&self->queue);
        if(aExcOccur()){ return; }

        /* call start */
        A_DEST(AAutoKey, key);
        AWorkQue_call(&self->call_queue);
        aExcClean(); key = AMtxCnd_lock(&self->lock);if(aExcOccur()){
            A_DEST(AWorkQue, self->call_queue);
            return;
        }
        /* call end   */

        AWorkQue_putQue(&self->queue, &self->call_queue);
        A_DEST(AWorkQue, self->call_queue);
        if(aExcOccur()){ return; }

        start = self->clock; now = A_INIT(AClock);
        auto awaken = ((int64_t)AWorkQue_next(&self->queue)) * 1000;
        auto awaken_clk = AClock_usAdd(start, awaken);
        if(AClock_usDiff(now, start) >= awaken){
        }else{
            while(self->stat){
                aExcClean(); AMtxCnd_timewait(&self->lock, awaken_clk); if(aExcOccur()){
                    if(aExcGet() == AEXC_timedout){
                        aExcClean();
                    }else{
                        return;
                    }
                }

                start = self->clock; now = A_INIT(AClock);
                awaken = ((int64_t)AWorkQue_next(&self->queue)) * 1000;
                awaken_clk = AClock_usAdd(start, awaken);
                if(AClock_usDiff(now, start) >= awaken){
                    break;
                }
            }
        }

        A_DEST(AAutoKey, key);
    }
}

static inline void ATimer_add(ATimer* self, AWork work){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }

    aExcClean();RAII(AAutoKey) key = AMtxCnd_lock(&self->lock);if(aExcOccur()){
        return;
    }

    if(!self->stat){
        aExcSet(AEXC_system_error);
        return;
    }

    if(AWorkQue_empty(&self->queue)){
        self->clock = A_INIT(AClock);
    }

    AClock start = self->clock;
    AClock now = A_INIT(AClock);
    AClock awaken_clk = AClock_usAdd(now, work.wait * 1000);
    work.wait = (uint32_t)(AClock_usDiff(awaken_clk, start) / 1000);

    AWorkQue_push(&self->queue, work);

    AMtxCnd_awake_all(&self->lock);
}

static inline void ATimer_rm(ATimer* self, int64_t id){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxCnd_lock(&self->lock);if(aExcOccur()){
        return;
    }

    if(!self->stat){
        aExcSet(AEXC_system_error);
        return;
    }

    if(self->call_queue.que.f != nullptr){
        forEach(it, self->call_queue.que){
            if(it.p != nullptr && it.p->id == id){
                AWork_set_rm(it.p);
                return;
            }
        }
    }
    AWorkQue_rm(&self->queue, id);
}
static inline int ATimer_thread(void* arg){
    aExcClean();
    ATimer* self = arg;
    ATimer_main(self);
    return aExcGet();
}
static inline void ATimer_start(ATimer* self){
     if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    aExcClean();RAII(AAutoKey) key = AMtxCnd_lock(&self->lock);if(aExcOccur()){
        return;
    }

    if(self->stat){
        return;
    }

    self->stat = true;
    if (thrd_create(&self->tid, ATimer_thread, self) != thrd_success) {
        aExcSet(AEXC_system_error);
        self->stat = false;
    }
}
static inline void ATimer_poweroff(ATimer* self){
     if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    aExcClean();auto key = AMtxCnd_lock(&self->lock);if(aExcOccur()){
        return;
    }

    if(!self->stat){
        return;
    }

    self->stat = false;
    AMtxCnd_awake_all(&self->lock);

    A_DEST(AAutoKey, key);

    thrd_join(self->tid, &self->thret);
}

static atomic_llong a_work_count = 0;
static ATimer a_timer_0;

/* 毫秒级定时器 */
__attribute__((constructor)) static inline void a_mstimer_start(){
    atomic_store_explicit(&a_work_count, 1, memory_order_relaxed);
    aExcClean(); a_timer_0 = A_INIT(ATimer);if(aExcOccur()){
        return;
    }
    ATimer_start(&a_timer_0);
}
__attribute__((destructor)) static inline void a_mstimer_poweroff(){
    ATimer_poweroff(&a_timer_0);
    A_DEST(ATimer, a_timer_0);
    atomic_store_explicit(&a_work_count, 0, memory_order_relaxed);
}
/* 移除任务 */
void a_timer_rmwork(int64_t id){
    ATimer_rm(&a_timer_0, id);
}

int64_t a_timer_addwork(uint32_t cycle, uint32_t num, void(*call)(void*), void* data){
    if(call == nullptr || cycle == 0 || num == 0){
        aExcSet(AEXC_outdomain); return -1;
    }
    AWork work = {
        .id = atomic_fetch_add(&a_work_count, 1),
        .wait = cycle, .cycle = cycle,
        .call = call, .data = data,
        .num = num,
    };
    aExcClean(); ATimer_add(&a_timer_0, work); if(aExcOccur()){
        return -1;
    }
    return work.id;
}


