/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __athrd_h__
#define __athrd_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __noreturn
    #if defined(__cplusplus)
        #define __noreturn [[noreturn]]
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
        #define __noreturn [[noreturn]]
    #elif defined(__GNUC__)
        #define __noreturn __attribute__((noreturn))
    #else
        #define __noreturn _Noreturn
    #endif /* __cplusplus */
#endif /* __noreturn */

#if !defined(__STDC_NO_THREADS__)
    #if defined(__has_include)
        #if __has_include(<threads.h>)
            #include <threads.h>
            #define A_THRD_USE_SYSTEM_THREADS 1
        #endif /* __has_include(<threads.h>) */
    #else
        #include <threads.h>
        #define A_THRD_USE_SYSTEM_THREADS 1
    #endif /* __has_include */
#endif /* !__STDC_NO_THREADS__ */

#if !defined(A_THRD_USE_SYSTEM_THREADS)
    #include <time.h>

    typedef int (*thrd_start_t)(void*);
    typedef void (*tss_dtor_t)(void*);

    enum{
        thrd_success = 0,
        thrd_busy = 1,
        thrd_error = 2,
        thrd_nomem = 3,
        thrd_timedout = 4,
    };

    enum{
        mtx_plain = 0,
        mtx_recursive = 1,
        mtx_timed = 2,
    };

    #define TSS_DTOR_ITERATIONS 4

    #if defined(_WIN32)
        #if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0600)
            #undef _WIN32_WINNT
            #define _WIN32_WINNT 0x0600
        #endif /* _WIN32_WINNT < 0x0600 */

        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN 1
        #endif /* WIN32_LEAN_AND_MEAN */

        #include <windows.h>

        #define A_THRD_USE_WIN32 1

        typedef struct{
            HANDLE handle;
            DWORD id;
            int owned;
        }thrd_t;

        typedef INIT_ONCE once_flag;
        #define ONCE_FLAG_INIT INIT_ONCE_STATIC_INIT

        typedef struct{
            int type;
            union{
                CRITICAL_SECTION critical_section;
                SRWLOCK srwlock;
            }native;
        }mtx_t;

        typedef CONDITION_VARIABLE cnd_t;

        typedef struct{
            DWORD slot;
            tss_dtor_t destructor;
        }tss_t;
    #elif defined(__C_POSIX__) || defined(_POSIX_THREADS) || defined(__unix__) \
            || defined(__APPLE__) || defined(__MACH__) || defined(__linux__)
        #include <pthread.h>

        #define A_THRD_USE_POSIX 1

        typedef pthread_t thrd_t;
        typedef pthread_once_t once_flag;
        #define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
        typedef pthread_mutex_t mtx_t;
        typedef pthread_cond_t cnd_t;
        typedef pthread_key_t tss_t;
    #else
        #error "The target platform cannot support multithreading"
    #endif /* _WIN32 */

    int thrd_create(thrd_t* thr, thrd_start_t func, void* arg);
    int thrd_equal(thrd_t lhs, thrd_t rhs);
    thrd_t thrd_current(void);
    int thrd_sleep(const struct timespec* duration, struct timespec* remaining);
    __noreturn void thrd_exit(int res);
    int thrd_detach(thrd_t thr);
    int thrd_join(thrd_t thr, int* res);
    void thrd_yield(void);

    int mtx_init(mtx_t* mutex, int type);
    int mtx_lock(mtx_t* mutex);
    int mtx_timedlock(mtx_t* mutex, const struct timespec* time_point);
    int mtx_trylock(mtx_t* mutex);
    int mtx_unlock(mtx_t* mutex);
    void mtx_destroy(mtx_t* mutex);

    void call_once(once_flag* flag, void (*func)(void));

    int cnd_init(cnd_t* cond);
    int cnd_signal(cnd_t* cond);
    int cnd_broadcast(cnd_t* cond);
    int cnd_wait(cnd_t* cond, mtx_t* mutex);
    int cnd_timedwait(cnd_t* cond, mtx_t* mutex, const struct timespec* time_point);
    void cnd_destroy(cnd_t* cond);

    int tss_create(tss_t* key, tss_dtor_t destructor);
    void* tss_get(tss_t key);
    int tss_set(tss_t key, void* val);
    void tss_delete(tss_t key);
#endif /* !A_THRD_USE_SYSTEM_THREADS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __athrd_h__ */
