/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __astack_h__
#define __astack_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"
#include "adeque.h"

#define AStack(T) __A_Splice(__A_Generic_Stack_$__, T)
#define __Ascf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_Stack_$__, T), __func_$__), name)

#define AStack_Define(T)                                                            \
    typedef struct AStack(T) AStack(T);                                             \
    typedef struct A_FUNC(AStack(T)) A_FUNC(AStack(T));                             \
    struct AStack(T){                                                               \
        const A_FUNC(AStack(T))* f;                                                 \
        __Adeq deq;                                                                 \
        T* type[0];                                                                 \
    };                                                                              \
                                                                                    \
    AIter_Define(AStack(T));                                                        \
                                                                                    \
    struct A_FUNC(AStack(T)){                                                       \
        bool    flag;                                                               \
        void    (*dest)(void*);                                                     \
        T*      (*const at)       (const AStack(T)* self, uint32_t index);          \
        void    (*const push) (AStack(T)* self, const T obj);                       \
        void    (*const pop)  (AStack(T)* self, T* tar);                            \
        uint32_t(*const getNumber)(const AStack(T)* self);                          \
        bool    (*const empty)    (const AStack(T)* self);                          \
        AIter(AStack(T)) (*const head)(const AStack(T)* self);                      \
        AIter(AStack(T)) (*const tail)(const AStack(T)* self);                      \
        void    (*const next)(AIter(AStack(T))* it);                                \
        void    (*const prev)(AIter(AStack(T))* it);                                \
    };                                                                              \
                                                                                    \
    static inline T* __Ascf(T,at)(const AStack(T)* self, uint32_t i);               \
    static inline void __Ascf(T,pushBack)(AStack(T)* self, const T obj);            \
    static inline void __Ascf(T,popBack)(AStack(T)* self, T* tar);                  \
    static inline uint32_t __Ascf(T,getNumber)(const AStack(T)* self);              \
    static inline bool __Ascf(T,empty)(const AStack(T)* self);                      \
    static inline AIter(AStack(T)) __Ascf(T, iter_head)(const AStack(T)* self);     \
    static inline AIter(AStack(T)) __Ascf(T, iter_tail)(const AStack(T)* self);     \
    static inline void __Ascf(T, iter_next)(AIter(AStack(T))* it);                  \
    static inline void __Ascf(T, iter_prev)(AIter(AStack(T))* it);                  \
    static inline void __A_OBJ_DEST_FUNC_SELF(AStack(T))(AStack(T)*);               \
    static inline void __A_OBJ_INIT_FUNC_SELF(AStack(T))(AStack(T)*);               \
    static inline void __A_OBJ_COPY_FUNC_SELF(AStack(T))(AStack(T)*, const AStack(T)*);     \
    static inline int __A_OBJ_CMPD_FUNC_SELF(AStack(T))(const AStack(T)*,const AStack(T)*); \
                                                                                    \
    static const A_FUNC(AStack(T)) A_FUNC_TAB(AStack(T)) = {                        \
        true,                                                                       \
        (void*)__A_OBJ_DEST_FUNC_SELF(AStack(T)),                                   \
        __Ascf(T,at),                                                               \
        __Ascf(T,pushBack),                                                         \
        __Ascf(T,popBack),                                                          \
        __Ascf(T,getNumber),                                                        \
        __Ascf(T,empty),                                                            \
        __Ascf(T,iter_head),                                                        \
        __Ascf(T,iter_tail),                                                        \
        __Ascf(T,iter_next),                                                        \
        __Ascf(T,iter_prev),                                                        \
    };                                                                              \

#define AStack_Generate(T)                                                          \
    static inline T* __Ascf(T,at)(const AStack(T)* self, uint32_t i){               \
        return __Adeq_at(&self->deq, i);                                            \
    }                                                                               \
                                                                                    \
    static inline void __Ascf(T,pushBack)(AStack(T)* self, const T obj){            \
        aExcClean();                                                                \
        T objx = A_COPY(T, obj); if(__a_unlikely(aExcOccur())){ return; }           \
        int ret = __Adeq_push_back(&self->deq); uint32_t i = self->deq.num - 1;     \
        if(__a_unlikely(ret != 0)){ aExcSet(ret); A_DEST(T,objx); return; }         \
        *__Ascf(T,at)(self, i) = objx;                                              \
    }                                                                               \
                                                                                    \
    static inline void __Ascf(T,popBack)(AStack(T)* self, T* tar){                  \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                               \
        uint32_t i = self->deq.num - 1;                                             \
        if(__a_unlikely(self->deq.num == 0)){ aExcSet(AEXC_overstep); return; }     \
        T obj = *((T*)__Adeq_at(&self->deq, i)); __Adeq_pop_back(&self->deq);       \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                         \
    }                                                                               \
                                                                                    \
    static inline uint32_t __Ascf(T,getNumber)(const AStack(T)* self){              \
        return self->deq.num;                                                       \
    }                                                                               \
                                                                                    \
    static inline bool __Ascf(T,empty)(const AStack(T)* self){                      \
        return self->deq.num == 0;                                                  \
    }                                                                               \
                                                                                    \
    static inline AIter(AStack(T)) __Ascf(T,iter_head)(const AStack(T)*self){       \
        AIter(AStack(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };        \
        if(self->deq.num != 0){                                                     \
            it.p = __Ascf(T,at)(self, 0);                                           \
            it.i = 0;                                                               \
        }                                                                           \
        return it;                                                                  \
    }                                                                               \
                                                                                    \
    static inline AIter(AStack(T)) __Ascf(T,iter_tail)(const AStack(T)*self){       \
        AIter(AStack(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };        \
        if(self->deq.num != 0){                                                     \
            it.p = __Ascf(T,at)(self, self->deq.num - 1);                           \
            it.i = self->deq.num - 1;                                               \
        }                                                                           \
        return it;                                                                  \
    }                                                                               \
                                                                                    \
    static inline void __Ascf(T,iter_next)(AIter(AStack(T))* it){                   \
        it->i++; it->p = __Ascf(T,at)(it->con, it->i);                              \
    }                                                                               \
                                                                                    \
    static inline void __Ascf(T,iter_prev)(AIter(AStack(T))* it){                   \
        it->i--; it->p = __Ascf(T,at)(it->con, it->i);                              \
    }                                                                               \
                                                                                    \
    __unused static void A_OBJ_DEST(AStack(T))(AStack(T)* self){                    \
        for(uint32_t i = 0; i < self->deq.num; i++){                                \
            A_DEST(T, *(T*)__Ascf(T,at)(self, i));                                  \
        }                                                                           \
        __Adeq_dest(&self->deq);                                                    \
    }                                                                               \
    __unused static void A_OBJ_INIT(AStack(T))(AStack(T)* self){                    \
        __Adeq_init(&self->deq, sizeof(T));                                         \
        self->f = &A_FUNC_TAB(AStack(T));                                           \
    }                                                                               \
    __unused static void A_OBJ_COPY(AStack(T))(AStack(T)* self, const AStack(T)* that){     \
        self->f = that->f;                                                          \
        __Adeq_init(&self->deq, sizeof(T));                                         \
        for(uint32_t i = 0; i < that->deq.num; i++){                                \
            aExcClean();                                                            \
            __Ascf(T,pushBack)(self, *__Ascf(T,at)(that, i));                       \
            if(aExcOccur()) return;                                                 \
        }                                                                           \
    }                                                                               \
    __unused static int A_OBJ_CMPD(AStack(T))(const AStack(T)*self,const AStack(T)*that){   \
        int ret = 0;                                                                \
        uint32_t num = self->deq.num <= that->deq.num ? self->deq.num:that->deq.num;\
        for(uint32_t i = 0; ret == 0 && i < num; i++){                              \
            ret = A_CMPD(T, *(T*)__Ascf(T,at)(self,i), *(T*)__Ascf(T,at)(that,i));  \
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

#endif /*__astack_h__*/

