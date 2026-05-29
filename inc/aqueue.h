/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __aqueue_h__
#define __aqueue_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"
#include "adeque.h"

#define AQueue(T) __A_Splice(__A_Generic_Queue_$__, T)
#define __Aquf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_Queue_$__, T), __func_$__), name)

#define AQueue_Define(T)                                                            \
    typedef struct AQueue(T) AQueue(T);                                             \
    typedef struct A_FUNC(AQueue(T)) A_FUNC(AQueue(T));                             \
    struct AQueue(T){                                                               \
        const A_FUNC(AQueue(T))* f;                                                 \
        __Adeq deq;                                                                 \
        T* type[0];                                                                 \
    };                                                                              \
                                                                                    \
    AIter_Define(AQueue(T));                                                        \
                                                                                    \
    struct A_FUNC(AQueue(T)){                                                       \
        bool    flag;                                                               \
        void    (*dest)(void*);                                                     \
        T*      (*const at)       (const AQueue(T)* self, uint32_t index);          \
        void    (*const push) (AQueue(T)* self, const T obj);                       \
        void    (*const pop) (AQueue(T)* self, T* tar);                             \
        uint32_t(*const getNumber)(const AQueue(T)* self);                          \
        bool    (*const empty)    (const AQueue(T)* self);                          \
        AIter(AQueue(T)) (*const head)(const AQueue(T)* self);                      \
        AIter(AQueue(T)) (*const tail)(const AQueue(T)* self);                      \
        void    (*const next)(AIter(AQueue(T))* it);                                \
        void    (*const prev)(AIter(AQueue(T))* it);                                \
    };                                                                              \
                                                                                    \
    static inline T* __Aquf(T,at)(const AQueue(T)* self, uint32_t i);               \
    static inline void __Aquf(T,pushBack)(AQueue(T)* self, const T obj);            \
    static inline void __Aquf(T,popFront)(AQueue(T)* self, T* tar);                 \
    static inline uint32_t __Aquf(T,getNumber)(const AQueue(T)* self);              \
    static inline bool __Aquf(T,empty)(const AQueue(T)* self);                      \
    static inline AIter(AQueue(T)) __Aquf(T, iter_head)(const AQueue(T)* self);     \
    static inline AIter(AQueue(T)) __Aquf(T, iter_tail)(const AQueue(T)* self);     \
    static inline void __Aquf(T, iter_next)(AIter(AQueue(T))* it);                  \
    static inline void __Aquf(T, iter_prev)(AIter(AQueue(T))* it);                  \
    static inline void __A_OBJ_DEST_FUNC_SELF(AQueue(T))(AQueue(T)*);               \
    static inline void __A_OBJ_INIT_FUNC_SELF(AQueue(T))(AQueue(T)*);               \
    static inline void __A_OBJ_COPY_FUNC_SELF(AQueue(T))(AQueue(T)*, const AQueue(T)*);     \
    static inline int __A_OBJ_CMPD_FUNC_SELF(AQueue(T))(const AQueue(T)*,const AQueue(T)*); \
                                                                                    \
    static const A_FUNC(AQueue(T)) A_FUNC_TAB(AQueue(T)) = {                        \
        true,                                                                       \
        (void*)__A_OBJ_DEST_FUNC_SELF(AQueue(T)),                                   \
        __Aquf(T,at),                                                               \
        __Aquf(T,pushBack),                                                         \
        __Aquf(T,popFront),                                                         \
        __Aquf(T,getNumber),                                                        \
        __Aquf(T,empty),                                                            \
        __Aquf(T,iter_head),                                                        \
        __Aquf(T,iter_tail),                                                        \
        __Aquf(T,iter_next),                                                        \
        __Aquf(T,iter_prev),                                                        \
    };                                                                              \

#define AQueue_Generate(T)                                                          \
    static inline T* __Aquf(T,at)(const AQueue(T)* self, uint32_t i){               \
        return __Adeq_at(&self->deq, i);                                            \
    }                                                                               \
                                                                                    \
    static inline void __Aquf(T,pushBack)(AQueue(T)* self, const T obj){            \
        aExcClean();                                                                \
        T objx = A_COPY(T, obj); if(__a_unlikely(aExcOccur())){ return; }           \
        int ret = __Adeq_push_back(&self->deq); uint32_t i = self->deq.num - 1;     \
        if(__a_unlikely(ret != 0)){ aExcSet(ret); A_DEST(T,objx); return; }         \
        *__Aquf(T,at)(self, i) = objx;                                              \
    }                                                                               \
                                                                                    \
    static inline void __Aquf(T,popFront)(AQueue(T)* self, T* tar){                 \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                               \
        uint32_t i = 0;                                                             \
        if(__a_unlikely(self->deq.num == 0)){ aExcSet(AEXC_overstep); return; }     \
        T obj = *((T*)__Adeq_at(&self->deq, i)); __Adeq_pop_front(&self->deq);      \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                         \
    }                                                                               \
                                                                                    \
    static inline uint32_t __Aquf(T,getNumber)(const AQueue(T)* self){              \
        return self->deq.num;                                                       \
    }                                                                               \
                                                                                    \
    static inline bool __Aquf(T,empty)(const AQueue(T)* self){                      \
        return self->deq.num == 0;                                                  \
    }                                                                               \
                                                                                    \
    static inline AIter(AQueue(T)) __Aquf(T,iter_head)(const AQueue(T)*self){       \
        AIter(AQueue(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };        \
        if(self->deq.num != 0){                                                     \
            it.p = __Aquf(T,at)(self, 0);                                           \
            it.i = 0;                                                               \
        }                                                                           \
        return it;                                                                  \
    }                                                                               \
                                                                                    \
    static inline AIter(AQueue(T)) __Aquf(T,iter_tail)(const AQueue(T)*self){       \
        AIter(AQueue(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };        \
        if(self->deq.num != 0){                                                     \
            it.p = __Aquf(T,at)(self, self->deq.num - 1);                           \
            it.i = self->deq.num - 1;                                               \
        }                                                                           \
        return it;                                                                  \
    }                                                                               \
                                                                                    \
    static inline void __Aquf(T,iter_next)(AIter(AQueue(T))* it){                   \
        it->i++; it->p = __Aquf(T,at)(it->con, it->i);                              \
    }                                                                               \
                                                                                    \
    static inline void __Aquf(T,iter_prev)(AIter(AQueue(T))* it){                   \
        it->i--; it->p = __Aquf(T,at)(it->con, it->i);                              \
    }                                                                               \
                                                                                    \
    __noused __weak __visibility(protected) void A_OBJ_DEST(AQueue(T))              \
    (AQueue(T)* self){                                                              \
        for(uint32_t i = 0; i < self->deq.num; i++){                                \
            A_DEST(T, *(T*)__Aquf(T,at)(self, i));                                  \
        }                                                                           \
        __Adeq_dest(&self->deq);                                                    \
    }                                                                               \
    __noused __weak __visibility(protected) void A_OBJ_INIT(AQueue(T))              \
    (AQueue(T)* self){                                                              \
        __Adeq_init(&self->deq, sizeof(T));                                         \
        self->f = &A_FUNC_TAB(AQueue(T));                                           \
    }                                                                               \
    __noused __weak __visibility(protected) void A_OBJ_COPY(AQueue(T))              \
    (AQueue(T)* self, const AQueue(T)* that){                                       \
        self->f = that->f;                                                          \
        __Adeq_init(&self->deq, sizeof(T));                                         \
        for(uint32_t i = 0; i < that->deq.num; i++){                                \
            aExcClean();                                                            \
            __Aquf(T,pushBack)(self, *__Aquf(T,at)(that, i));                       \
            if(aExcOccur()) return;                                                 \
        }                                                                           \
    }                                                                               \
    __noused __weak __visibility(protected) int A_OBJ_CMPD(AQueue(T))               \
    (const AQueue(T)*self,const AQueue(T)*that){                                    \
        int ret = 0;                                                                \
        uint32_t num = self->deq.num <= that->deq.num ? self->deq.num:that->deq.num;\
        for(uint32_t i = 0; ret == 0 && i < num; i++){                              \
            ret = A_CMPD(T, *(T*)__Aquf(T,at)(self,i), *(T*)__Aquf(T,at)(that,i));  \
        }                                                                           \
        if(ret == 0 && self->deq.num != that->deq.num){                             \
            if(self->deq.num > that->deq.num) ret = 1;                              \
            if(self->deq.num < that->deq.num) ret = -1;                             \
        }                                                                           \
        return ret;                                                                 \
    }                                                                               \


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__aqueue_h__*/

