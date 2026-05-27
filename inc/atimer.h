/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __atimer_h__
#define __atimer_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include <time.h>

#ifndef TIME_UTC
    #define TIME_UTC 1
#endif /* TIME_UTC */

#if !defined(__timespec_defined) && !defined(_STRUCT_TIMESPEC) && \
	!defined(_TIMESPEC_DECLARED) && !defined(HAVE_STRUCT_TIMESPEC)
    struct timespec{
        time_t  tv_sec;
        long    tv_nsec;
    };
#endif /* timespec */
__weak int timespec_get(struct timespec *ts, int base);



typedef struct{
    struct timespec clock;
}AClock;
__weak void A_OBJ_INIT(AClock)(AClock* self){
   if(TIME_UTC != timespec_get(&self->clock, TIME_UTC)){
        aExcSet(AEXC_system_error);
   }
}
__weak void A_OBJ_COPY(AClock)(AClock* self, __noused const AClock* that){
    A_OBJ_INIT(AClock)(self);
}
static inline int64_t AClock_nsDiff(AClock self, AClock that);
__weak int A_OBJ_CMPD(AClock)(const AClock* self, const AClock* that){
    auto ret = AClock_nsDiff(*self, *that);
    return ret == 0 ? 0 : (ret > 0 ? 1 : -1);
}
A_TYPE_REGISTER(AClock);

static inline int64_t AClock_nsDiff(AClock self, AClock that){
    int64_t ns = (int64_t)(self.clock.tv_sec) - (int64_t)(that.clock.tv_sec);
    ns *= 1000 * 1000 * 1000;
    ns += (int64_t)(self.clock.tv_nsec) - (int64_t)(that.clock.tv_nsec);
    return ns;
}
static inline int64_t AClock_usDiff(AClock self, AClock that){
    return AClock_nsDiff(self, that) / 1000;
}
static inline int64_t AClock_msDiff(AClock self, AClock that){
    return AClock_usDiff(self, that) / 1000;
}
static inline int64_t AClock_sDiff(AClock self, AClock that){
    return AClock_msDiff(self, that) / 1000;
}
static inline void AClock_refresh(AClock* self){
   if(TIME_UTC != timespec_get(&self->clock, TIME_UTC)){
        aExcSet(AEXC_system_error);
   }
}

static inline AClock AClock_nsAdd(AClock self, int64_t t){
    t += self.clock.tv_nsec; t += self.clock.tv_sec * (1000 * 1000 * 1000);
    self.clock.tv_sec = 0, self.clock.tv_nsec = 0;
    auto s = t / (1000 * 1000 * 1000); auto ns = t % (1000 * 1000 * 1000);
    self.clock.tv_sec += s, self.clock.tv_nsec += ns;
    return self;
}
static inline AClock AClock_usAdd(AClock self, int64_t t){
    return AClock_nsAdd(self, t * 1000);
}
static inline AClock AClock_msAdd(AClock self, int64_t t){
    return AClock_usAdd(self, t * 1000);
}
static inline AClock AClock_sAdd(AClock self, int64_t t){
    return AClock_msAdd(self, t * 1000);
}

static inline AClock AClock_nsCvs(int64_t t){
    return (AClock){
        .clock = {
            .tv_sec = (time_t)(t / (1000 * 1000 * 1000)),
            .tv_nsec = (long)(t % (1000 * 1000 * 1000)),
        }
    };
}
static inline AClock AClock_usCvs(int64_t t){
    return AClock_nsCvs(t * 1000);
}
static inline AClock AClock_msCvs(int64_t t){
    return AClock_usCvs(t * 1000);
}
static inline AClock AClock_sCvs(int64_t t){
    return AClock_msCvs(t * 1000);
}





/* 毫秒级定时器 */
void a_timer_rmwork(int64_t id);
int64_t a_timer_addwork(uint32_t cycle/*周期*/, uint32_t num/*执行次数*/, void(*call)(void*), void* data);
static const uint32_t aLongWork = 0xffffffff;

static inline int64_t a_timer_addwork_long(uint32_t cycle/*周期*/, void(*call)(void*), void* data){
    return a_timer_addwork(cycle, aLongWork, call, data);
}

static inline int64_t a_timer_addwork_one(uint32_t cycle/*周期*/, void(*call)(void*), void* data){
    return a_timer_addwork(cycle, 1, call, data);
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__atimer_h__*/

