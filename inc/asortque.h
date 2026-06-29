/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __asortque_h__
#define __asortque_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"
#include "aline.h"

#define ASortque(T) __A_Splice(__A_Generic_Sortque_$__, T)
#define __Asqf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_Sortque_$__, T), __func_$__), name)

#define ASortque_Define(T)                                                              \
    typedef struct ASortque(T) ASortque(T);                                             \
    typedef struct A_FUNC(ASortque(T)) A_FUNC(ASortque(T));                             \
    struct ASortque(T){                                                                 \
        const A_FUNC(ASortque(T))* f;                                                   \
        __Aarr arr;                                                                     \
        T* p;                                                                           \
        T* type[0];                                                                     \
    };                                                                                  \
                                                                                        \
    AIter_Define(ASortque(T));                                                          \
                                                                                        \
    struct A_FUNC(ASortque(T)){                                                         \
        bool    flag;                                                                   \
        void    (*dest)(void*);                                                         \
        T*      (*const at)       (const ASortque(T)* self, uint32_t index);            \
        void    (*const rm)       (ASortque(T)* self, uint32_t index);                  \
        void    (*const ins)      (ASortque(T)* self, const T s);                       \
        void    (*const take)     (ASortque(T)* self, uint32_t index, T*target);        \
        void    (*const popMax)   (ASortque(T)* self, T* target);                       \
        void    (*const popMin)   (ASortque(T)* self, T* target);                       \
        uint32_t(*const getNumber)(const ASortque(T)* self);                            \
        bool    (*const empty)    (const ASortque(T)* self);                            \
        AIter(ASortque(T)) (*const head)(const ASortque(T)* self);                      \
        AIter(ASortque(T)) (*const tail)(const ASortque(T)* self);                      \
        void    (*const next)(AIter(ASortque(T))* it);                                  \
        void    (*const prev)(AIter(ASortque(T))* it);                                  \
    };                                                                                  \
                                                                                        \
    static inline T*   __Asqf(T,at)(const ASortque(T)* self, uint32_t i);               \
    static inline void __Asqf(T,rm)(ASortque(T)* self, uint32_t i);                     \
    static inline void __Asqf(T,ins)(ASortque(T)*self, const T obj);                    \
    static inline void __Asqf(T,take)(ASortque(T)* self, uint32_t i, T* tar);           \
    static inline void __Asqf(T,popBack)(ASortque(T)* self, T* tar);                    \
    static inline void __Asqf(T,popFront)(ASortque(T)* self, T* tar);                   \
    static inline uint32_t __Asqf(T,getNumber)(const ASortque(T)* self);                \
    static inline bool __Asqf(T,empty)(const ASortque(T)* self);                        \
    static inline AIter(ASortque(T)) __Asqf(T, iter_head)(const ASortque(T)* self);     \
    static inline AIter(ASortque(T)) __Asqf(T, iter_tail)(const ASortque(T)* self);     \
    static inline void __Asqf(T, iter_next)(AIter(ASortque(T))* it);                    \
    static inline void __Asqf(T, iter_prev)(AIter(ASortque(T))* it);                    \
    static inline void __A_OBJ_DEST_FUNC_SELF(ASortque(T))(ASortque(T)*);               \
    static inline bool __A_OBJ_INIT_FUNC_SELF(ASortque(T))(ASortque(T)*);               \
    static inline bool __A_OBJ_COPY_FUNC_SELF(ASortque(T))(ASortque(T)*, const ASortque(T)*);    \
    static inline int __A_OBJ_CMPD_FUNC_SELF(ASortque(T))(const ASortque(T)*,const ASortque(T)*);\
                                                                                        \
    static const A_FUNC(ASortque(T)) A_FUNC_TAB(ASortque(T)) = {                        \
        true,                                                                           \
        (void*)__A_OBJ_DEST_FUNC_SELF(ASortque(T)),                                     \
        __Asqf(T,at),                                                                   \
        __Asqf(T,rm),                                                                   \
        __Asqf(T,ins),                                                                  \
        __Asqf(T,take),                                                                 \
        __Asqf(T,popBack),                                                              \
        __Asqf(T,popFront),                                                             \
        __Asqf(T,getNumber),                                                            \
        __Asqf(T,empty),                                                                \
        __Asqf(T,iter_head),                                                            \
        __Asqf(T,iter_tail),                                                            \
        __Asqf(T,iter_next),                                                            \
        __Asqf(T,iter_prev),                                                            \
    };                                                                                  \

#define ASortque_Generate(T)                                                            \
    static inline uint32_t __Asqf(T,find)(const ASortque(T)*self, const T obj){         \
        /* 二分查找 */                                                                  \
        if(__a_unlikely(self->arr.num == 0)){ return 0; }                               \
        if(A_CMPD(T, obj, *__Asqf(T,at)(self, 0)) < 0){ return 0; }                     \
        if(A_CMPD(T,obj,*__Asqf(T,at)(self,self->arr.num-1))>=0){return self->arr.num;} \
                                                                                        \
        uint32_t x = self->arr.num / 2, i = self->arr.num / 2;                          \
                                                                                        \
        while(x > 16){                                                                  \
            x = x / 2;                                                                  \
            int cmp = A_CMPD(T, obj, *__Asqf(T,at)(self, i));                           \
            if(cmp == 0){                                                               \
                break;                                                                  \
            }else if(cmp < 0){                                                          \
                i -= x;                                                                 \
            }else{                                                                      \
                i += x;                                                                 \
            }                                                                           \
        }                                                                               \
                                                                                        \
        if(A_CMPD(T, obj, *__Asqf(T,at)(self, i)) >= 0){                                \
            /* v >= [i] *//* 向上查找 */                                                \
            for(;i < self->arr.num; i++){                                               \
                if(A_CMPD(T, obj, *__Asqf(T,at)(self, i)) < 0){                         \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
        }else{                                                                          \
            /* v < [i] *//* 向下查找 */                                                 \
            i++;                                                                        \
            for(;i > 0; i--){                                                           \
                if(A_CMPD(T, obj, *__Asqf(T,at)(self, i - 1)) >= 0){                    \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
        }                                                                               \
                                                                                        \
        return i;                                                                       \
    }                                                                                   \
                                                                                        \
    static inline void __Asqf(T,ins_i)(ASortque(T)*self, uint32_t i, const T obj){      \
        if(i > self->arr.num) i = self->arr.num ;                                       \
        aTry(T objx = A_COPY(T, obj);)aExc{ return; }                                   \
        int ret = __Aarr_ins(&self->arr, i); self->p = __Aarr_get_strat(&self->arr);    \
        if(__a_unlikely(ret != 0)){ aErrSet(ret); A_DEST(T, objx); return; }            \
        self->p[i] = objx;                                                              \
    }                                                                                   \
                                                                                        \
    static inline void __Asqf(T,ins)(ASortque(T)*self, const T obj){                    \
        uint32_t i = __Asqf(T,find)(self, obj);                                         \
        __Asqf(T,ins_i)(self, i, obj);                                                  \
    }                                                                                   \
                                                                                        \
    static inline T* __Asqf(T,at)(const ASortque(T)* self, uint32_t i){                 \
        if(__a_unlikely(i == AEND))i = self->arr.num - 1 ;                              \
        if(__a_unlikely(i >= self->arr.num)) return nullptr;                            \
        return &(self->p[i]);                                                           \
    }                                                                                   \
                                                                                        \
    static inline void __Asqf(T,rm)(ASortque(T)* self, uint32_t i){                     \
        if(__a_unlikely(self->arr.num == 0)) return;                                    \
        if(i >= self->arr.num){ i = self->arr.num - 1; } A_DEST(T, self->p[i]);         \
        __Aarr_rm(&self->arr, i); self->p = __Aarr_get_strat(&self->arr);               \
    }                                                                                   \
                                                                                        \
                                                                                        \
    static inline void __Asqf(T,take)(ASortque(T)* self, uint32_t i, T* tar){           \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                                   \
        if(__a_unlikely(self->arr.num == 0)){ aErrSet(AERR_overstep); return; }         \
        if(i >= self->arr.num){ i = self->arr.num - 1; } T obj = self->p[i];            \
        __Aarr_rm(&self->arr, i);self->p = __Aarr_get_strat(&self->arr);                \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                             \
    }                                                                                   \
                                                                                        \
    static inline void __Asqf(T,popBack)(ASortque(T)* self, T* tar){                    \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                                   \
        if(__a_unlikely(self->arr.num == 0)){ aErrSet(AERR_overstep); return; }         \
        uint32_t i = self->arr.num - 1; T obj = self->p[i];                             \
        __Aarr_pop_back(&self->arr); self->p = __Aarr_get_strat(&self->arr);            \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                             \
    }                                                                                   \
                                                                                        \
    static inline void __Asqf(T,popFront)(ASortque(T)* self, T* tar){                   \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                                   \
        if(__a_unlikely(self->arr.num == 0)){ aErrSet(AERR_overstep); return; }         \
        uint32_t i = 0; T obj = self->p[i];                                             \
        __Aarr_pop_front(&self->arr); self->p = __Aarr_get_strat(&self->arr);           \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                             \
    }                                                                                   \
                                                                                        \
    static inline uint32_t __Asqf(T,getNumber)(const ASortque(T)* self){                \
        return self->arr.num;                                                           \
    }                                                                                   \
                                                                                        \
    static inline bool __Asqf(T,empty)(const ASortque(T)* self){                        \
        return self->arr.num == 0;                                                      \
    }                                                                                   \
                                                                                        \
    static inline AIter(ASortque(T)) __Asqf(T, iter_head)(const ASortque(T)* self){     \
        AIter(ASortque(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };          \
        if(self->arr.num != 0){                                                         \
            it.p = self->p, it.i = 0;                                                   \
        }                                                                               \
        return it;                                                                      \
    }                                                                                   \
    static inline AIter(ASortque(T)) __Asqf(T, iter_tail)(const ASortque(T)* self){     \
        AIter(ASortque(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };          \
        if(self->arr.num != 0){                                                         \
            it.p = self->p + self->arr.num - 1, it.i = self->arr.num - 1;               \
        }                                                                               \
        return it;                                                                      \
    }                                                                                   \
    static inline void __Asqf(T, iter_next)(AIter(ASortque(T))* it){                    \
        it->p++, it->i++;                                                               \
    }                                                                                   \
    static inline void __Asqf(T, iter_prev)(AIter(ASortque(T))* it){                    \
        it->p--, it->i--;                                                               \
    }                                                                                   \
                                                                                        \
    __noused static inline void A_OBJ_DEST                                              \
    (ASortque(T))(ASortque(T)* self){                                                   \
        for(uint32_t i = 0; i < self->arr.num; i++){                                    \
            A_DEST(T, self->p[i]);                                                      \
        }                                                                               \
        __Aarr_dest(&self->arr);                                                        \
    }                                                                                   \
    __noused static inline void A_OBJ_INIT                                              \
    (ASortque(T))(ASortque(T)* self){                                                   \
        self->arr.size = sizeof(T); self->f = &A_FUNC_TAB(ASortque(T));                 \
    }                                                                                   \
    __noused static inline void A_OBJ_COPY(ASortque(T))                                 \
    (ASortque(T)* self, const ASortque(T)* that){                                       \
        self->f = that->f;                                                              \
        int ret = __Aarr_copy(&self->arr, &that->arr);                                  \
        self->p = __Aarr_get_strat(&self->arr);                                         \
        if(ret != 0){ aErrSet(AERR_alloc_failed); return; }                             \
        for(uint32_t i = 0; i < that->arr.num; i++){                                    \
            aTry(self->p[i] = A_COPY(T, that->p[i]);)aExc{ return; }                    \
            self->arr.num++;                                                            \
        }                                                                               \
    }                                                                                   \
    __noused static inline int  A_OBJ_CMPD(ASortque(T))                                 \
    (const ASortque(T)* self,const ASortque(T)*that){                                   \
        int ret = 0;                                                                    \
        uint32_t num = self->arr.num <= that->arr.num ? self->arr.num : that->arr.num;  \
        for(uint32_t i = 0; i < num && ret == 0; i++){                                  \
            ret = A_CMPD(T, self->p[i], that->p[i]);                                    \
        }                                                                               \
        if(ret == 0 && self->arr.num != that->arr.num){                                 \
            if(self->arr.num > that->arr.num) ret = 1;                                  \
            if(self->arr.num < that->arr.num) ret = -1;                                 \
        }                                                                               \
        return ret;                                                                     \
    }                                                                                   \
                                                                                        \

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __asortque_h__ */
