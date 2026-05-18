/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <athrd.h>

#if !defined(A_THRD_USE_SYSTEM_THREADS)

    #include <errno.h>
    #include <limits.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>

static inline int __athrd_timespec_valid(const struct timespec* ts){
    return ts != NULL && ts->tv_sec >= 0 && ts->tv_nsec >= 0 && ts->tv_nsec < 1000000000L;
}

static inline int __athrd_timespec_cmp(const struct timespec* lhs, const struct timespec* rhs){
    if(lhs->tv_sec < rhs->tv_sec){ return -1; }
    if(lhs->tv_sec > rhs->tv_sec){ return 1; }
    if(lhs->tv_nsec < rhs->tv_nsec){ return -1; }
    if(lhs->tv_nsec > rhs->tv_nsec){ return 1; }
    return 0;
}

static inline struct timespec __athrd_timespec_sub(const struct timespec* end,
        const struct timespec* begin){
    struct timespec diff = {
        .tv_sec = end->tv_sec - begin->tv_sec,
        .tv_nsec = end->tv_nsec - begin->tv_nsec,
    };

    if(diff.tv_nsec < 0){
        diff.tv_sec--;
        diff.tv_nsec += 1000000000L;
    }

    if(diff.tv_sec < 0){
        diff.tv_sec = 0;
        diff.tv_nsec = 0;
    }
    return diff;
}

    #if defined(__C_POSIX__)

        #include <sched.h>

typedef struct{
    thrd_start_t func;
    void* arg;
}__athrd_start_ctx_t;

static inline int __athrd_posix_status(int err){
    switch(err){
        case 0: return thrd_success;
        case EBUSY: return thrd_busy;
        case ETIMEDOUT: return thrd_timedout;
        case EAGAIN:
        case ENOMEM: return thrd_nomem;
        default: return thrd_error;
    }
}

static void* __athrd_posix_start(void* arg){
    __athrd_start_ctx_t* ctx = arg;
    thrd_start_t func = ctx->func;
    void* user_arg = ctx->arg;
    free(ctx);
    return (void*)(intptr_t)func(user_arg);
}

static int __athrd_posix_now_utc(struct timespec* now){
    #if defined(CLOCK_REALTIME)
        return clock_gettime(CLOCK_REALTIME, now);
    #else
        return timespec_get(now, TIME_UTC) == TIME_UTC ? 0 : -1;
    #endif /* CLOCK_REALTIME */
}

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg){
    if(thr == NULL || func == NULL){ return thrd_error; }

    __athrd_start_ctx_t* ctx = malloc(sizeof(*ctx));
    if(ctx == NULL){ return thrd_nomem; }

    ctx->func = func;
    ctx->arg = arg;

    int err = pthread_create(thr, NULL, __athrd_posix_start, ctx);
    if(err != 0){
        free(ctx);
        return __athrd_posix_status(err);
    }
    return thrd_success;
}

int thrd_equal(thrd_t lhs, thrd_t rhs){
    return pthread_equal(lhs, rhs);
}

thrd_t thrd_current(void){
    return pthread_self();
}

int thrd_sleep(const struct timespec* duration, struct timespec* remaining){
    if(remaining != NULL){
        memset(remaining, 0, sizeof(*remaining));
    }
    if(!__athrd_timespec_valid(duration)){ return -2; }

    if(nanosleep(duration, remaining) == 0){ return 0; }
    if(errno == EINTR){ return -1; }

    if(remaining != NULL){
        memset(remaining, 0, sizeof(*remaining));
    }
    return -2;
}

void thrd_exit(int res){
    pthread_exit((void*)(intptr_t)res);
    abort();
}

int thrd_detach(thrd_t thr){
    return __athrd_posix_status(pthread_detach(thr));
}

int thrd_join(thrd_t thr, int* res){
    void* ret = NULL;
    int err = pthread_join(thr, &ret);
    if(err != 0){ return __athrd_posix_status(err); }

    if(res != NULL){
        *res = (int)(intptr_t)ret;
    }
    return thrd_success;
}

void thrd_yield(void){
    (void)sched_yield();
}

int mtx_init(mtx_t* mutex, int type){
    if(mutex == NULL){ return thrd_error; }

    pthread_mutexattr_t attr;
    int err = pthread_mutexattr_init(&attr);
    if(err != 0){ return __athrd_posix_status(err); }

    if((type & mtx_recursive) != 0){
        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if(err != 0){
            pthread_mutexattr_destroy(&attr);
            return __athrd_posix_status(err);
        }
    }

    err = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return __athrd_posix_status(err);
}

int mtx_lock(mtx_t* mutex){
    if(mutex == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_mutex_lock(mutex));
}

int mtx_timedlock(mtx_t* mutex, const struct timespec* time_point){
    if(mutex == NULL || !__athrd_timespec_valid(time_point)){ return thrd_error; }

    for(;;){
        int err = pthread_mutex_trylock(mutex);
        if(err == 0){ return thrd_success; }
        if(err != EBUSY){ return __athrd_posix_status(err); }

        struct timespec now;
        if(__athrd_posix_now_utc(&now) != 0){ return thrd_error; }
        if(__athrd_timespec_cmp(&now, time_point) >= 0){ return thrd_timedout; }

        struct timespec pause = __athrd_timespec_sub(time_point, &now);
        if(pause.tv_sec != 0 || pause.tv_nsec > 1000000L){
            pause.tv_sec = 0;
            pause.tv_nsec = 1000000L;
        }

        if(nanosleep(&pause, NULL) != 0 && errno != EINTR){
            return thrd_error;
        }
    }
}

int mtx_trylock(mtx_t* mutex){
    if(mutex == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_mutex_trylock(mutex));
}

int mtx_unlock(mtx_t* mutex){
    if(mutex == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_mutex_unlock(mutex));
}

void mtx_destroy(mtx_t* mutex){
    if(mutex == NULL){ return; }
    (void)pthread_mutex_destroy(mutex);
}

void call_once(once_flag* flag, void (*func)(void)){
    if(flag == NULL || func == NULL){ return; }
    (void)pthread_once(flag, func);
}

int cnd_init(cnd_t* cond){
    if(cond == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_cond_init(cond, NULL));
}

int cnd_signal(cnd_t* cond){
    if(cond == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_cond_signal(cond));
}

int cnd_broadcast(cnd_t* cond){
    if(cond == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_cond_broadcast(cond));
}

int cnd_wait(cnd_t* cond, mtx_t* mutex){
    if(cond == NULL || mutex == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_cond_wait(cond, mutex));
}

int cnd_timedwait(cnd_t* cond, mtx_t* mutex, const struct timespec* time_point){
    if(cond == NULL || mutex == NULL || !__athrd_timespec_valid(time_point)){
        return thrd_error;
    }
    return __athrd_posix_status(pthread_cond_timedwait(cond, mutex, time_point));
}

void cnd_destroy(cnd_t* cond){
    if(cond == NULL){ return; }
    (void)pthread_cond_destroy(cond);
}

int tss_create(tss_t* key, tss_dtor_t destructor){
    if(key == NULL){ return thrd_error; }
    return __athrd_posix_status(pthread_key_create(key, destructor));
}

void* tss_get(tss_t key){
    return pthread_getspecific(key);
}

int tss_set(tss_t key, void* val){
    return __athrd_posix_status(pthread_setspecific(key, val));
}

void tss_delete(tss_t key){
    pthread_key_delete(key);
}
    #elif defined(_WIN32)

        #include <process.h>

typedef struct{
    thrd_start_t func;
    void* arg;
}__athrd_start_ctx_t;

typedef struct{
    void* value;
    tss_dtor_t destructor;
}__athrd_tss_entry_t;

static inline void __athrd_win32_now_utc(struct timespec* now){
    FILETIME ft;
    ULARGE_INTEGER ticks;
    const uint64_t unix_epoch_ticks = UINT64_C(116444736000000000);

    GetSystemTimeAsFileTime(&ft);
    ticks.LowPart = ft.dwLowDateTime;
    ticks.HighPart = ft.dwHighDateTime;
    ticks.QuadPart -= unix_epoch_ticks;

    now->tv_sec = (time_t)(ticks.QuadPart / UINT64_C(10000000));
    now->tv_nsec = (long)((ticks.QuadPart % UINT64_C(10000000)) * UINT64_C(100));
}

static inline DWORD __athrd_win32_timeout_ms(const struct timespec* time_point, int* expired){
    struct timespec now;
    uint64_t timeout_ms;
    struct timespec delta;

    __athrd_win32_now_utc(&now);
    if(__athrd_timespec_cmp(&now, time_point) >= 0){
        *expired = 1;
        return 0;
    }

    *expired = 0;
    delta = __athrd_timespec_sub(time_point, &now);

    if((uint64_t)delta.tv_sec >= (UINT64_C(0xffffffff) - 1) / UINT64_C(1000)){
        return INFINITE - 1;
    }

    timeout_ms = (uint64_t)delta.tv_sec * UINT64_C(1000);
    timeout_ms += ((uint64_t)delta.tv_nsec + UINT64_C(999999)) / UINT64_C(1000000);
    if(timeout_ms >= UINT64_C(0xffffffff)){
        timeout_ms = UINT64_C(0xffffffff) - 1;
    }
    return (DWORD)timeout_ms;
}

static inline int __athrd_win32_duration_ticks(const struct timespec* duration, int64_t* ticks){
    uint64_t total;
    uint64_t sec;

    if(!__athrd_timespec_valid(duration)){ return 0; }

    sec = (uint64_t)duration->tv_sec;
    if(sec > (uint64_t)INT64_MAX / UINT64_C(10000000)){ return 0; }

    total = sec * UINT64_C(10000000);
    total += ((uint64_t)duration->tv_nsec + UINT64_C(99)) / UINT64_C(100);
    if(total > (uint64_t)INT64_MAX){ return 0; }

    *ticks = (int64_t)total;
    return 1;
}

static inline uint64_t __athrd_win32_duration_ms(const struct timespec* duration){
    uint64_t sec = (uint64_t)duration->tv_sec;
    uint64_t millis;

    if(sec > UINT64_MAX / UINT64_C(1000)){ return UINT64_MAX; }

    millis = sec * UINT64_C(1000);
    millis += ((uint64_t)duration->tv_nsec + UINT64_C(999999)) / UINT64_C(1000000);
    return millis;
}

static inline void __athrd_win32_sleep_ms(uint64_t timeout_ms){
    while(timeout_ms != 0){
        DWORD chunk = timeout_ms >= UINT64_C(0xffffffff)
                ? INFINITE - 1
                : (DWORD)timeout_ms;
        Sleep(chunk);
        timeout_ms -= chunk;
    }
}

static unsigned __stdcall __athrd_win32_start(void* arg){
    __athrd_start_ctx_t* ctx = arg;
    thrd_start_t func = ctx->func;
    void* user_arg = ctx->arg;
    int ret;
    free(ctx);

    ret = func(user_arg);
    return (unsigned)ret;
}

static VOID NTAPI __athrd_win32_tss_cleanup(void* data){
    __athrd_tss_entry_t* entry = data;
    if(entry == NULL){ return; }

    void* value = entry->value;
    tss_dtor_t destructor = entry->destructor;
    free(entry);

    if(value != NULL && destructor != NULL){
        destructor(value);
    }
}

static BOOL CALLBACK __athrd_win32_call_once(PINIT_ONCE init_once, PVOID param, PVOID* ctx){
    void (*func)(void) = (void (*)(void))param;
    (void)init_once;
    (void)ctx;
    func();
    return TRUE;
}

static inline int __athrd_win32_is_recursive(const mtx_t* mutex){
    return (mutex->type & mtx_recursive) != 0;
}

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg){
    unsigned thread_id = 0;
    uintptr_t handle;
    __athrd_start_ctx_t* ctx;

    if(thr == NULL || func == NULL){ return thrd_error; }

    ctx = malloc(sizeof(*ctx));
    if(ctx == NULL){ return thrd_nomem; }

    ctx->func = func;
    ctx->arg = arg;

    handle = _beginthreadex(NULL, 0, __athrd_win32_start, ctx, 0, &thread_id);
    if(handle == 0){
        free(ctx);
        return (errno == ENOMEM || errno == EAGAIN) ? thrd_nomem : thrd_error;
    }

    thr->handle = (HANDLE)handle;
    thr->id = (DWORD)thread_id;
    thr->owned = 1;
    return thrd_success;
}

int thrd_equal(thrd_t lhs, thrd_t rhs){
    return lhs.id == rhs.id;
}

thrd_t thrd_current(void){
    return (thrd_t){
        .handle = GetCurrentThread(),
        .id = GetCurrentThreadId(),
        .owned = 0,
    };
}

int thrd_sleep(const struct timespec* duration, struct timespec* remaining){
    int64_t ticks;
    HANDLE timer;
    LARGE_INTEGER due_time;

    if(remaining != NULL){
        memset(remaining, 0, sizeof(*remaining));
    }
    if(!__athrd_win32_duration_ticks(duration, &ticks)){ return -2; }
    if(ticks == 0){ return 0; }

    timer = CreateWaitableTimerW(NULL, TRUE, NULL);
    if(timer != NULL){
        due_time.QuadPart = -(LONGLONG)ticks;
        if(SetWaitableTimer(timer, &due_time, 0, NULL, NULL, FALSE) != 0){
            DWORD wait = WaitForSingleObject(timer, INFINITE);
            CloseHandle(timer);
            return wait == WAIT_OBJECT_0 ? 0 : -2;
        }
        CloseHandle(timer);
    }

    __athrd_win32_sleep_ms(__athrd_win32_duration_ms(duration));
    return 0;
}

void thrd_exit(int res){
    _endthreadex((unsigned)res);
    abort();
}

int thrd_detach(thrd_t thr){
    if(thr.handle == NULL || !thr.owned){ return thrd_error; }
    return CloseHandle(thr.handle) != 0 ? thrd_success : thrd_error;
}

int thrd_join(thrd_t thr, int* res){
    DWORD exit_code = 0;
    DWORD wait;

    if(thr.handle == NULL || !thr.owned){ return thrd_error; }
    if(thrd_equal(thr, thrd_current())){ return thrd_error; }

    wait = WaitForSingleObject(thr.handle, INFINITE);
    if(wait != WAIT_OBJECT_0){ return thrd_error; }
    if(GetExitCodeThread(thr.handle, &exit_code) == 0){
        CloseHandle(thr.handle);
        return thrd_error;
    }

    CloseHandle(thr.handle);
    if(res != NULL){
        *res = (int)exit_code;
    }
    return thrd_success;
}

void thrd_yield(void){
    SwitchToThread();
}

int mtx_init(mtx_t* mutex, int type){
    if(mutex == NULL){ return thrd_error; }

    memset(mutex, 0, sizeof(*mutex));
    mutex->type = type;

    if(__athrd_win32_is_recursive(mutex)){
        InitializeCriticalSection(&mutex->native.critical_section);
    }else{
        InitializeSRWLock(&mutex->native.srwlock);
    }
    return thrd_success;
}

int mtx_lock(mtx_t* mutex){
    if(mutex == NULL){ return thrd_error; }

    if(__athrd_win32_is_recursive(mutex)){
        EnterCriticalSection(&mutex->native.critical_section);
    }else{
        AcquireSRWLockExclusive(&mutex->native.srwlock);
    }
    return thrd_success;
}

int mtx_timedlock(mtx_t* mutex, const struct timespec* time_point){
    if(mutex == NULL || !__athrd_timespec_valid(time_point)){ return thrd_error; }

    for(;;){
        int ret = mtx_trylock(mutex);
        if(ret != thrd_busy){ return ret; }

        int expired = 0;
        DWORD timeout = __athrd_win32_timeout_ms(time_point, &expired);
        if(expired){ return thrd_timedout; }

        if(timeout > 1){
            Sleep(1);
        }else{
            SwitchToThread();
        }
    }
}

int mtx_trylock(mtx_t* mutex){
    if(mutex == NULL){ return thrd_error; }

    if(__athrd_win32_is_recursive(mutex)){
        return TryEnterCriticalSection(&mutex->native.critical_section) != 0
                ? thrd_success
                : thrd_busy;
    }

    return TryAcquireSRWLockExclusive(&mutex->native.srwlock) != 0
            ? thrd_success
            : thrd_busy;
}

int mtx_unlock(mtx_t* mutex){
    if(mutex == NULL){ return thrd_error; }

    if(__athrd_win32_is_recursive(mutex)){
        LeaveCriticalSection(&mutex->native.critical_section);
    }else{
        ReleaseSRWLockExclusive(&mutex->native.srwlock);
    }
    return thrd_success;
}

void mtx_destroy(mtx_t* mutex){
    if(mutex == NULL){ return; }
    if(__athrd_win32_is_recursive(mutex)){
        DeleteCriticalSection(&mutex->native.critical_section);
    }
}

void call_once(once_flag* flag, void (*func)(void)){
    if(flag == NULL || func == NULL){ return; }
    (void)InitOnceExecuteOnce(flag, __athrd_win32_call_once, (PVOID)func, NULL);
}

int cnd_init(cnd_t* cond){
    if(cond == NULL){ return thrd_error; }
    InitializeConditionVariable(cond);
    return thrd_success;
}

int cnd_signal(cnd_t* cond){
    if(cond == NULL){ return thrd_error; }
    WakeConditionVariable(cond);
    return thrd_success;
}

int cnd_broadcast(cnd_t* cond){
    if(cond == NULL){ return thrd_error; }
    WakeAllConditionVariable(cond);
    return thrd_success;
}

int cnd_wait(cnd_t* cond, mtx_t* mutex){
    BOOL ok;

    if(cond == NULL || mutex == NULL){ return thrd_error; }

    if(__athrd_win32_is_recursive(mutex)){
        ok = SleepConditionVariableCS(cond, &mutex->native.critical_section, INFINITE);
    }else{
        ok = SleepConditionVariableSRW(cond, &mutex->native.srwlock, INFINITE, 0);
    }

    return ok != 0 ? thrd_success : thrd_error;
}

int cnd_timedwait(cnd_t* cond, mtx_t* mutex, const struct timespec* time_point){
    BOOL ok;
    int expired;
    DWORD timeout;

    if(cond == NULL || mutex == NULL || !__athrd_timespec_valid(time_point)){
        return thrd_error;
    }

    timeout = __athrd_win32_timeout_ms(time_point, &expired);
    if(expired){ return thrd_timedout; }

    if(__athrd_win32_is_recursive(mutex)){
        ok = SleepConditionVariableCS(cond, &mutex->native.critical_section, timeout);
    }else{
        ok = SleepConditionVariableSRW(cond, &mutex->native.srwlock, timeout, 0);
    }

    if(ok != 0){ return thrd_success; }
    return GetLastError() == ERROR_TIMEOUT ? thrd_timedout : thrd_error;
}

void cnd_destroy(cnd_t* cond){
    (void)cond;
}

int tss_create(tss_t* key, tss_dtor_t destructor){
    if(key == NULL){ return thrd_error; }

    key->slot = FlsAlloc(__athrd_win32_tss_cleanup);
    if(key->slot == FLS_OUT_OF_INDEXES){
        return thrd_error;
    }

    key->destructor = destructor;
    return thrd_success;
}

void* tss_get(tss_t key){
    __athrd_tss_entry_t* entry = FlsGetValue(key.slot);
    return entry != NULL ? entry->value : NULL;
}

int tss_set(tss_t key, void* val){
    __athrd_tss_entry_t* entry;

    if(key.slot == FLS_OUT_OF_INDEXES){ return thrd_error; }

    entry = FlsGetValue(key.slot);
    if(val == NULL){
        if(entry != NULL){
            free(entry);
            if(FlsSetValue(key.slot, NULL) == 0){
                return thrd_error;
            }
        }
        return thrd_success;
    }

    if(entry == NULL){
        entry = malloc(sizeof(*entry));
        if(entry == NULL){ return thrd_nomem; }

        entry->destructor = key.destructor;
        if(FlsSetValue(key.slot, entry) == 0){
            free(entry);
            return thrd_error;
        }
    }

    entry->value = val;
    return thrd_success;
}

void tss_delete(tss_t key){
    if(key.slot == FLS_OUT_OF_INDEXES){ return; }

    __athrd_tss_entry_t* entry = FlsGetValue(key.slot);
    if(entry != NULL){
        free(entry);
        FlsSetValue(key.slot, NULL);
    }
    FlsFree(key.slot);
}

    #endif /* __C_POSIX__ || _WIN32 */
#endif /* !A_THRD_USE_SYSTEM_THREADS */
