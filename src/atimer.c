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
    bool(*call)(void* data);    //任务处理函数
    void*       data;           //任务数据
    int64_t     id;             //任务id
    uint32_t    num;            //执行次数, =-1时为长期任务
    uint32_t    cycle;          //执行周期,ms
    uint32_t    wait;           //等待时间,ms,=0时立即执行
    atomic_int  rm_flag;        //被删除
}ATWork;
__noused static inline int A_OBJ_CMPD(ATWork)(const ATWork* self, const ATWork* that){
    int ret = A_CMPD(uint32_t, self->wait, that->wait);
    if(ret == 0) ret = A_CMPD(int64_t, self->id, that->id);
    return ret;
}
A_TYPE_REGISTER(ATWork);

static inline void ATWork_call(ATWork* self){
    if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }
    if(0 == atomic_load_explicit(&self->rm_flag, memory_order_relaxed)){
        bool ret = self->call(self->data);
        self->wait = self->cycle;   //重置等待时间
        if(self->num != aLongWork){
            if(__a_likely(self->num != 0)){
                self->num--;
            }
        }
        if(!ret){
            self->num = 0;
        }
    }
}
static inline void ATWork_set_rm(ATWork* self){
    if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }
    atomic_store_explicit(&self->rm_flag, 1, memory_order_relaxed);
}
__noused static inline void ATWork_clean_rm(ATWork* self){
    if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }
    atomic_store_explicit(&self->rm_flag, 0, memory_order_relaxed);
}



ASortque_Define(ATWork);
ASortque_Generate(ATWork);
A_TYPE_REGISTER(ASortque(ATWork));

typedef struct{
    ASortque(ATWork) que;
}ATWorkQue;
__noused static inline void A_OBJ_INIT(ATWorkQue)(ATWorkQue* self){
    self->que = A_INIT(ASortque(ATWork));
}
__noused static inline void A_OBJ_DEST(ATWorkQue)(ATWorkQue* self){
    A_DEST(ASortque(ATWork), self->que);
}
__noused static inline void A_OBJ_COPY(ATWorkQue)(ATWorkQue* self, __noused const ATWorkQue* that){
    self->que = A_COPY(ASortque(ATWork), that->que);
}
__noused static inline int A_OBJ_CMPD(ATWorkQue)(const ATWorkQue* self, const ATWorkQue* that){
    return A_CMPD(ASortque(ATWork), self->que, that->que);
}
A_TYPE_REGISTER(ATWorkQue);

static inline void ATWorkQue_rm(ATWorkQue* self, int64_t id){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }
    if(__a_unlikely(id < 0)){
        aErrSet(AERR_outdomain);
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
static inline void ATWorkQue_push(ATWorkQue* self, ATWork work){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    auto que = &self->que;
    que->f->ins(que, work);
}
static inline ATWork ATWorkQue_pop(ATWorkQue* self){
    ATWork work = {};
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return work;
    }

    auto que = &self->que;
    que->f->popMin(que, &work);
    return work;
}
static inline void ATWorkQue_updataWait(ATWorkQue* self, uint32_t time){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
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
static inline ATWorkQue ATWorkQue_getCallQue(ATWorkQue* self){
    auto wq = A_INIT(ATWorkQue);

    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return wq;
    }
    uint32_t n = self->que.f->getNumber(&self->que);
    for(uint32_t i = 0; i < n; i++){
        aErrClean();
        ATWork work = ATWorkQue_pop(self);
        if(aErrOccur()){
            break;
        }

        if(work.wait != 0){
            ATWorkQue_push(self, work);
            break;
        }else{
            ATWorkQue_push(&wq, work);
            if(aErrOccur()){
                break;
            }
        }
    }

    return wq;
}
static inline void ATWorkQue_putQue(ATWorkQue* self, ATWorkQue* that){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    uint32_t n = that->que.f->getNumber(&that->que);
    for(uint32_t i = 0; i < n; i++){
        aErrClean();
        ATWork work = ATWorkQue_pop(that);
        if(aErrOccur()){
            break;
        }

        if(work.rm_flag == 0 && work.num != 0){
            ATWorkQue_push(self, work);
            if(aErrOccur()){
                break;
            }
        }
    }
}
static inline bool ATWorkQue_empty(const ATWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return true;
    }
    auto que = &self->que;
    return que->f->empty(que);
}
static inline uint32_t ATWorkQue_next(const ATWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return true;
    }
    auto que = &self->que;
    if(que->f->getNumber(que) != 0){
        auto p = que->f->at(que, 0);
        return p->wait;
    }
    return 0xffffff;
}
static inline void ATWorkQue_call(const ATWorkQue* self){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    auto que = &self->que;
    forEach(it, *que){
        auto work = it.p;
        aErrClean();
        ATWork_call(work);
        aErrClean();
    }
}



/* 毫秒级定时器 */
typedef struct{
    thrd_t      tid;
    AMtxCnd     lock;
    AClock      clock;
    ATWorkQue queue;
    ATWorkQue call_queue;
    bool stat;
    int thret;
}ATimer;
__noused static inline void A_OBJ_INIT(ATimer)(ATimer* self){
    aErrClean();
    self->lock = A_INIT(AMtxCnd); if(aErrOccur()){ return; }
    self->clock = A_INIT(AClock); if(aErrOccur()){ return; }
    self->queue = A_INIT(ATWorkQue); if(aErrOccur()){ return; }
    self->call_queue = A_INIT(ATWorkQue); if(aErrOccur()){ return; }
    self->stat = false;
    self->thret = 0;
}
__noused static inline void A_OBJ_DEST(ATimer)(ATimer* self){
    A_DEST(AClock, self->clock);
    A_DEST(AMtxCnd, self->lock);
    A_DEST(ATWorkQue, self->queue);
    A_DEST(ATWorkQue, self->call_queue);
    self->stat = false;
    self->thret = 0;
}
__noused static inline void A_OBJ_COPY(ATimer)(ATimer* self, const ATimer* that){
    self->lock = A_INIT(AMtxCnd); if(aErrOccur()){ return; }
    self->clock = A_INIT(AClock); if(aErrOccur()){ return; }
    self->queue = A_COPY(ATWorkQue, that->queue); if(aErrOccur()){ return; }
    self->call_queue = A_COPY(ATWorkQue, that->call_queue); if(aErrOccur()){ return; }
    self->stat = false;
    self->thret = 0;
}
__noused static inline int A_OBJ_CMPD(ATimer)(const ATimer* self, const ATimer* that){
    return  A_CMPD(ATWorkQue, self->queue, that->queue);
}
A_TYPE_REGISTER(ATimer);

static inline void ATimer_main(ATimer* self){
    if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }
    while(1){
        aTry(auto key = AMtxCnd_lock(&self->lock);)aExc{
            return;
        }

        if(!self->stat){
            A_DEST(AAutoKey, key);
            return;
        }

        while(self->stat && ATWorkQue_empty(&self->queue)){
            aTry(AMtxCnd_wait(&self->lock);)aExc{
                A_DEST(AAutoKey, key);
                return;
            }
        }

        AClock start = self->clock;
        AClock now = A_INIT(AClock);
        self->clock = now;

        ATWorkQue_updataWait(&self->queue, (uint32_t)(AClock_usDiff(now, start) / 1000));
        if(aErrOccur()){ return; }

        self->call_queue = ATWorkQue_getCallQue(&self->queue);
        if(aErrOccur()){ return; }

        /* call start */
        A_DEST(AAutoKey, key);
        ATWorkQue_call(&self->call_queue);
        aTry(key = AMtxCnd_lock(&self->lock);)aExc{
            A_DEST(ATWorkQue, self->call_queue);
            return;
        }
        /* call end   */

        ATWorkQue_putQue(&self->queue, &self->call_queue);
        A_DEST(ATWorkQue, self->call_queue);
        if(aErrOccur()){ return; }

        start = self->clock; now = A_INIT(AClock);
        auto awaken = ((int64_t)ATWorkQue_next(&self->queue)) * 1000;
        auto awaken_clk = AClock_usAdd(start, awaken);
        if(AClock_usDiff(now, start) >= awaken){
        }else{
            while(self->stat){
                aTry(AMtxCnd_timewait(&self->lock, awaken_clk);)aHit(AERR_timedout){
                    aErrClean();
                }aExc{
                    return;
                }

                start = self->clock; now = A_INIT(AClock);
                awaken = ((int64_t)ATWorkQue_next(&self->queue)) * 1000;
                awaken_clk = AClock_usAdd(start, awaken);
                if(AClock_usDiff(now, start) >= awaken){
                    break;
                }
            }
        }

        A_DEST(AAutoKey, key);
    }
}

static inline void ATimer_add(ATimer* self, ATWork work){
    if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }

    aTry(RAII(AAutoKey) key = AMtxCnd_lock(&self->lock);)aExc{
        return;
    }

    if(!self->stat){
        aErrSet(AERR_system_error);
        return;
    }

    if(ATWorkQue_empty(&self->queue)){
        self->clock = A_INIT(AClock);
    }

    AClock start = self->clock;
    AClock now = A_INIT(AClock);
    AClock awaken_clk = AClock_usAdd(now, work.wait * 1000);
    work.wait = (uint32_t)(AClock_usDiff(awaken_clk, start) / 1000);

    ATWorkQue_push(&self->queue, work);

    AMtxCnd_awake_all(&self->lock);
}

static inline void ATimer_rm(ATimer* self, int64_t id){
    if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }

    aTry(RAII(AAutoKey) key = AMtxCnd_lock(&self->lock);)aExc{
        return;
    }

    if(!self->stat){
        aErrSet(AERR_system_error);
        return;
    }

    if(self->call_queue.que.f != nullptr){
        forEach(it, self->call_queue.que){
            if(it.p != nullptr && it.p->id == id){
                ATWork_set_rm(it.p);
                return;
            }
        }
    }
    ATWorkQue_rm(&self->queue, id);
}
static inline int ATimer_thread(void* arg){
    aErrClean();
    ATimer* self = arg;
    ATimer_main(self);
    return aErrGet();
}
static inline void ATimer_start(ATimer* self){
     if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }
    aTry(RAII(AAutoKey) key = AMtxCnd_lock(&self->lock);)aExc{
        return;
    }

    if(self->stat){
        return;
    }

    self->stat = true;
    if (thrd_create(&self->tid, ATimer_thread, self) != thrd_success) {
        aErrSet(AERR_system_error);
        self->stat = false;
    }
}
static inline void ATimer_poweroff(ATimer* self){
     if(self == nullptr){
        aErrSet(AERR_nullptr);
        return;
    }
    aTry(auto key = AMtxCnd_lock(&self->lock);)aExc{
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
static ATimer a_timer_0;

/* 毫秒级定时器 */
bool a_mstimer_start(void){
    atomic_store_explicit(&a_work_count, 1, memory_order_relaxed);
    aTry(a_timer_0 = A_INIT(ATimer);)aExc{
        return false;
    }
    aTry(ATimer_start(&a_timer_0);)aExc{
        return false;
    }
    return true;
}
void a_mstimer_poweroff(void){
    ATimer_poweroff(&a_timer_0);
    A_DEST(ATimer, a_timer_0);
    atomic_store_explicit(&a_work_count, 0, memory_order_relaxed);
}
/* 移除任务 */
void a_timer_rmwork(int64_t id){
    ATimer_rm(&a_timer_0, id);
}

int64_t a_timer_addwork(uint32_t cycle, uint32_t num, bool(*call)(void*), void* data){
    if(call == nullptr || cycle == 0 || num == 0){
        aErrSet(AERR_outdomain); return -1;
    }
    ATWork work = {
        .id = atomic_fetch_add(&a_work_count, 1),
        .wait = cycle, .cycle = cycle,
        .call = call, .data = data,
        .num = num,
    };
    aTry(ATimer_add(&a_timer_0, work);)aExc{
        return -1;
    }
    return work.id;
}


