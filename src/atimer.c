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
#include <sys/time.h>
__weak int timespec_get(struct timespec *ts, int base){
    if(base != TIME_UTC){
        return 0;
    }
    if(clock_gettime(CLOCK_REALTIME, ts) == 0){
        return base;
    }else{
        return 0;
    }
}
#endif /* __C_POSIX__ */

#if defined (__C_WINDOWS__) && !defined(_UCRT)
#include <windows.h>
int timespec_get(struct timespec *ts, int base){
    if(base != TIME_UTC){
        return 0;
    }

    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

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
#endif /* __C_WINDOWS__ */



typedef struct{
    void(*call)(void* data);    //任务处理函数
    void*       data;           //任务数据
    int64_t     id;             //任务id
    uint32_t    num;            //执行次数, =-1时为长期任务
    uint32_t    cycle;          //执行周期,ms
    uint32_t    wait;           //等待时间,ms,=0时立即执行
    atomic_int  rm_flag;        //被删除
}__ATWork;
A_TYPE_REGISTER(__ATWork);
__visibility(hidden) int A_OBJ_CMPD(__ATWork)(const __ATWork* self, const __ATWork* that){
    int ret = A_CMPD(uint32_t, self->wait, that->wait);
    if(ret == 0) ret = A_CMPD(int64_t, self->id, that->id);
    return ret;
}

static inline void __ATWork_call(__ATWork* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(0 == atomic_load_explicit(&self->rm_flag, memory_order_relaxed)){
        self->call(self->data);
        self->wait = self->cycle;   //重置等待时间
        if(self->num != aLongWork){
            if(__a_likely(self->num != 0)){
                self->num--;
            }
        }
    }
}
static inline void __ATWork_set_rm(__ATWork* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    atomic_store_explicit(&self->rm_flag, 1, memory_order_relaxed);
}
__noused static inline void __ATWork_clean_rm(__ATWork* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    atomic_store_explicit(&self->rm_flag, 0, memory_order_relaxed);
}



ASortque_Define(__ATWork);
ASortque_Generate(__ATWork);
A_TYPE_REGISTER(ASortque(__ATWork));

typedef struct{
    ASortque(__ATWork) que;
}__ATWorkQue;
A_TYPE_REGISTER(__ATWorkQue);
__visibility(hidden) void A_OBJ_INIT(__ATWorkQue)(__ATWorkQue* self){
    self->que = A_INIT(ASortque(__ATWork));
}
__visibility(hidden) void A_OBJ_DEST(__ATWorkQue)(__ATWorkQue* self){
    A_DEST(ASortque(__ATWork), self->que);
}
__visibility(hidden) void A_OBJ_COPY(__ATWorkQue)(__ATWorkQue* self, __noused const __ATWorkQue* that){
    self->que = A_COPY(ASortque(__ATWork), that->que);
}
__visibility(hidden) int A_OBJ_CMPD(__ATWorkQue)(const __ATWorkQue* self, const __ATWorkQue* that){
    return A_CMPD(ASortque(__ATWork), self->que, that->que);
}

static inline void __ATWorkQue_rm(__ATWorkQue* self, int64_t id){
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
static inline void __ATWorkQue_push(__ATWorkQue* self, __ATWork work){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto que = &self->que;
    que->f->ins(que, work);
}
static inline __ATWork __ATWorkQue_pop(__ATWorkQue* self){
    __ATWork work = {};
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return work;
    }

    auto que = &self->que;
    que->f->popMin(que, &work);
    return work;
}
static inline void __ATWorkQue_updataWait(__ATWorkQue* self, uint32_t time){
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
static inline __ATWorkQue __ATWorkQue_getCallQue(__ATWorkQue* self){
    auto wq = A_INIT(__ATWorkQue);

    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return wq;
    }
    uint32_t n = self->que.f->getNumber(&self->que);
    for(uint32_t i = 0; i < n; i++){
        aExcClean();
        __ATWork work = __ATWorkQue_pop(self);
        if(aExcOccur()){
            break;
        }

        if(work.wait != 0){
            __ATWorkQue_push(self, work);
            break;
        }else{
            __ATWorkQue_push(&wq, work);
            if(aExcOccur()){
                break;
            }
        }
    }

    return wq;
}
static inline void __ATWorkQue_putQue(__ATWorkQue* self, __ATWorkQue* that){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    uint32_t n = that->que.f->getNumber(&that->que);
    for(uint32_t i = 0; i < n; i++){
        aExcClean();
        __ATWork work = __ATWorkQue_pop(that);
        if(aExcOccur()){
            break;
        }

        if(work.rm_flag == 0 && work.num != 0){
            __ATWorkQue_push(self, work);
            if(aExcOccur()){
                break;
            }
        }
    }
}
static inline bool __ATWorkQue_empty(const __ATWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return true;
    }
    auto que = &self->que;
    return que->f->empty(que);
}
static inline uint32_t __ATWorkQue_next(const __ATWorkQue* self){
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
static inline void __ATWorkQue_call(const __ATWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto que = &self->que;
    forEach(it, *que){
        auto work = it.p;
        aExcClean();
        __ATWork_call(work);
        aExcClean();
    }
}



/* 毫秒级定时器 */
typedef struct{
    thrd_t      tid;
    AMtxCnd     lock;
    AClock      clock;
    __ATWorkQue queue;
    __ATWorkQue call_queue;
    bool stat;
    int thret;
}__ATimer;
A_TYPE_REGISTER(__ATimer);
__visibility(hidden) void A_OBJ_INIT(__ATimer)(__ATimer* self){
    aExcClean();
    self->lock = A_INIT(AMtxCnd); if(aExcOccur()){ return; }
    self->clock = A_INIT(AClock); if(aExcOccur()){ return; }
    self->queue = A_INIT(__ATWorkQue); if(aExcOccur()){ return; }
    self->call_queue = A_INIT(__ATWorkQue); if(aExcOccur()){ return; }
    self->stat = false;
    self->thret = 0;
}
__visibility(hidden) void A_OBJ_DEST(__ATimer)(__ATimer* self){
    A_DEST(AClock, self->clock);
    A_DEST(AMtxCnd, self->lock);
    A_DEST(__ATWorkQue, self->queue);
    A_DEST(__ATWorkQue, self->call_queue);
    self->stat = false;
    self->thret = 0;
}
__visibility(hidden) void A_OBJ_COPY(__ATimer)(__ATimer* self, const __ATimer* that){
    self->lock = A_INIT(AMtxCnd); if(aExcOccur()){ return; }
    self->clock = A_INIT(AClock); if(aExcOccur()){ return; }
    self->queue = A_COPY(__ATWorkQue, that->queue); if(aExcOccur()){ return; }
    self->call_queue = A_COPY(__ATWorkQue, that->call_queue); if(aExcOccur()){ return; }
    self->stat = false;
    self->thret = 0;
}
__visibility(hidden) int A_OBJ_CMPD(__ATimer)(const __ATimer* self, const __ATimer* that){
    return  A_CMPD(__ATWorkQue, self->queue, that->queue);
}

static inline void __ATimer_main(__ATimer* self){
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

        while(self->stat && __ATWorkQue_empty(&self->queue)){
            aExcClean(); AMtxCnd_wait(&self->lock); if(aExcOccur()){
                A_DEST(AAutoKey, key);
                return;
            }
        }

        AClock start = self->clock;
        AClock now = A_INIT(AClock);
        self->clock = now;

        __ATWorkQue_updataWait(&self->queue, (uint32_t)(AClock_usDiff(now, start) / 1000));
        if(aExcOccur()){ return; }

        self->call_queue = __ATWorkQue_getCallQue(&self->queue);
        if(aExcOccur()){ return; }

        /* call start */
        A_DEST(AAutoKey, key);
        __ATWorkQue_call(&self->call_queue);
        aExcClean(); key = AMtxCnd_lock(&self->lock);if(aExcOccur()){
            A_DEST(__ATWorkQue, self->call_queue);
            return;
        }
        /* call end   */

        __ATWorkQue_putQue(&self->queue, &self->call_queue);
        A_DEST(__ATWorkQue, self->call_queue);
        if(aExcOccur()){ return; }

        start = self->clock; now = A_INIT(AClock);
        auto awaken = ((int64_t)__ATWorkQue_next(&self->queue)) * 1000;
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
                awaken = ((int64_t)__ATWorkQue_next(&self->queue)) * 1000;
                awaken_clk = AClock_usAdd(start, awaken);
                if(AClock_usDiff(now, start) >= awaken){
                    break;
                }
            }
        }

        A_DEST(AAutoKey, key);
    }
}

static inline void __ATimer_add(__ATimer* self, __ATWork work){
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

    if(__ATWorkQue_empty(&self->queue)){
        self->clock = A_INIT(AClock);
    }

    AClock start = self->clock;
    AClock now = A_INIT(AClock);
    AClock awaken_clk = AClock_usAdd(now, work.wait * 1000);
    work.wait = (uint32_t)(AClock_usDiff(awaken_clk, start) / 1000);

    __ATWorkQue_push(&self->queue, work);

    AMtxCnd_awake_all(&self->lock);
}

static inline void __ATimer_rm(__ATimer* self, int64_t id){
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
                __ATWork_set_rm(it.p);
                return;
            }
        }
    }
    __ATWorkQue_rm(&self->queue, id);
}
static inline int __ATimer_thread(void* arg){
    aExcClean();
    __ATimer* self = arg;
    __ATimer_main(self);
    return aExcGet();
}
static inline void __ATimer_start(__ATimer* self){
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
    if (thrd_create(&self->tid, __ATimer_thread, self) != thrd_success) {
        aExcSet(AEXC_system_error);
        self->stat = false;
    }
}
static inline void __ATimer_poweroff(__ATimer* self){
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

#if __SIZEOF_POINTER__ == 8
static atomic_llong a_work_count = 0;
#else
static atomic_long a_work_count = 0;
#endif /* 64 or 32 */
static __ATimer a_timer_0;

/* 毫秒级定时器 */
__attribute__((constructor)) static inline void a_mstimer_start(){
    atomic_store_explicit(&a_work_count, 1, memory_order_relaxed);
    aExcClean(); a_timer_0 = A_INIT(__ATimer);if(aExcOccur()){
        return;
    }
    __ATimer_start(&a_timer_0);
}
__attribute__((destructor)) static inline void a_mstimer_poweroff(){
    __ATimer_poweroff(&a_timer_0);
    A_DEST(__ATimer, a_timer_0);
    atomic_store_explicit(&a_work_count, 0, memory_order_relaxed);
}
/* 移除任务 */
void a_timer_rmwork(int64_t id){
    __ATimer_rm(&a_timer_0, id);
}

int64_t a_timer_addwork(uint32_t cycle, uint32_t num, void(*call)(void*), void* data){
    if(call == nullptr || cycle == 0 || num == 0){
        aExcSet(AEXC_outdomain); return -1;
    }
    __ATWork work = {
        .id = atomic_fetch_add(&a_work_count, 1),
        .wait = cycle, .cycle = cycle,
        .call = call, .data = data,
        .num = num,
    };
    aExcClean(); __ATimer_add(&a_timer_0, work); if(aExcOccur()){
        return -1;
    }
    return work.id;
}


