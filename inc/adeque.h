/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __adeque_h__
#define __adeque_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"

typedef struct{
    char** data;
    uint32_t cap;
    uint32_t num;
    uint32_t size;
    uint32_t offset;
    uint32_t blk_size;
}__A2arr;

__noused static inline void* __A2arr_at(const __A2arr* arr, uint32_t i){
    uint32_t x = i / arr->blk_size;
    uint32_t y = i % arr->blk_size;
    return &(arr->data[x + arr->offset][y * arr->size]);
}
__noused static inline uint32_t __A2arr_cap(__A2arr* arr){
    return arr->num * arr->blk_size;
}

void __A2arr_init(__A2arr* arr, uint32_t size);
void __A2arr_dest(__A2arr* arr);
int __A2arr_add_blk_back (__A2arr* arr);
int __A2arr_add_blk_front(__A2arr* arr);
int __A2arr_sub_blk_back (__A2arr* arr);
int __A2arr_sub_blk_front(__A2arr* arr);


typedef struct{
    __A2arr arr;
    uint32_t num;
    uint32_t offset;
}__Adeq;

__noused static inline void* __Adeq_at(const __Adeq* deq, uint32_t i){
    if(__a_unlikely(i >= deq->num)) i = deq->num - 1 ;
    if(__a_unlikely(deq->num == 0)){
        aExcSet(AEXC_overstep);
        return nullptr;
    }
    return __A2arr_at(&deq->arr, i + deq->offset);
}

static inline void __Adeq_init(__Adeq* deq, uint32_t size){
    __A2arr_init(&deq->arr, size);
}
static inline void __Adeq_dest(__Adeq* deq){
    __A2arr_dest(&deq->arr);
}
static inline int __Adeq_push_back(__Adeq* deq){
    int ret = 0;
    uint32_t num = deq->offset + deq->num;
    if(__a_unlikely(num == __A2arr_cap(&deq->arr))){
        ret = __A2arr_add_blk_back(&deq->arr);
        if(__a_unlikely(ret != 0)) return AEXC_alloc_failed;
    }
    deq->num++;
    return 0;
}
static inline int __Adeq_push_front(__Adeq* deq){
    int ret = 0;
    if(__a_unlikely(deq->offset == 0)){
        ret = __A2arr_add_blk_front(&deq->arr);
        if(__a_unlikely(ret != 0)) return AEXC_alloc_failed;
        deq->offset = deq->arr.blk_size;
    }
    deq->offset--, deq->num++;
    return 0;
}
static inline void __Adeq_pop_back(__Adeq* deq){
    uint32_t num = deq->offset + deq->num;
    if(__a_unlikely(__A2arr_cap(&deq->arr) >= num + deq->arr.blk_size)){
        __A2arr_sub_blk_back(&deq->arr);
    }
    deq->num--;
}
static inline void __Adeq_pop_front(__Adeq* deq){
    if(__a_unlikely(deq->offset >= deq->arr.blk_size)){
        /* sub 失败意味着什么也不做，因此仅当sub成功时才调整offset*/
        if(__a_likely(0 == __A2arr_sub_blk_front(&deq->arr))){
            deq->offset -= deq->arr.blk_size;
        }
    }
    deq->offset++, deq->num--;
}

#define ADeque(T) __A_Splice(__A_Generic_Deque_$__, T)
#define __Adqf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_Deque_$__, T), __func_$__), name)

#define ADeque_Define(T)                                                            \
    typedef struct ADeque(T) ADeque(T);                                             \
    typedef struct A_FUNC(ADeque(T)) A_FUNC(ADeque(T));                             \
    struct ADeque(T){                                                               \
        const A_FUNC(ADeque(T))* f;                                                 \
        __Adeq deq;                                                                 \
        T* type[0];                                                                 \
    };                                                                              \
                                                                                    \
    AIter_Define(ADeque(T));                                                        \
                                                                                    \
    struct A_FUNC(ADeque(T)){                                                       \
        bool    flag;                                                               \
        void    (*dest)(void*);                                                     \
        T*      (*const at)       (const ADeque(T)* self, uint32_t index);          \
        void    (*const pushBack) (ADeque(T)* self, const T obj);                   \
        void    (*const pushFront)(ADeque(T)* self, const T obj);                   \
        void    (*const popBack)  (ADeque(T)* self, T* tar);                        \
        void    (*const popFront) (ADeque(T)* self, T* tar);                        \
        uint32_t(*const getNumber)(const ADeque(T)* self);                          \
        bool    (*const empty)    (const ADeque(T)* self);                          \
        AIter(ADeque(T)) (*const head)(const ADeque(T)* self);                      \
        AIter(ADeque(T)) (*const tail)(const ADeque(T)* self);                      \
        void    (*const next)(AIter(ADeque(T))* it);                                \
        void    (*const prev)(AIter(ADeque(T))* it);                                \
    };                                                                              \
                                                                                    \
    static inline T* __Adqf(T,at)(const ADeque(T)* self, uint32_t i);               \
    static inline void __Adqf(T,pushBack)(ADeque(T)* self, const T obj);            \
    static inline void __Adqf(T,pushFront)(ADeque(T)* self, const T obj);           \
    static inline void __Adqf(T,popBack)(ADeque(T)* self, T* tar);                  \
    static inline void __Adqf(T,popFront)(ADeque(T)* self, T* tar);                 \
    static inline uint32_t __Adqf(T,getNumber)(const ADeque(T)* self);              \
    static inline bool __Adqf(T,empty)(const ADeque(T)* self);                      \
    static inline AIter(ADeque(T)) __Adqf(T, iter_head)(const ADeque(T)* self);     \
    static inline AIter(ADeque(T)) __Adqf(T, iter_tail)(const ADeque(T)* self);     \
    static inline void __Adqf(T, iter_next)(AIter(ADeque(T))* it);                  \
    static inline void __Adqf(T, iter_prev)(AIter(ADeque(T))* it);                  \
    static inline void __A_OBJ_DEST_FUNC_SELF(ADeque(T))(ADeque(T)*);               \
    static inline void __A_OBJ_INIT_FUNC_SELF(ADeque(T))(ADeque(T)*);               \
    static inline void __A_OBJ_COPY_FUNC_SELF(ADeque(T))(ADeque(T)*, const ADeque(T)*);     \
    static inline int __A_OBJ_CMPD_FUNC_SELF(ADeque(T))(const ADeque(T)*,const ADeque(T)*); \
                                                                                    \
    static const A_FUNC(ADeque(T)) A_FUNC_TAB(ADeque(T)) = {                        \
        true,                                                                       \
        (void*)__A_OBJ_DEST_FUNC_SELF(ADeque(T)),                                   \
        __Adqf(T,at),                                                               \
        __Adqf(T,pushBack),                                                         \
        __Adqf(T,pushFront),                                                        \
        __Adqf(T,popBack),                                                          \
        __Adqf(T,popFront),                                                         \
        __Adqf(T,getNumber),                                                        \
        __Adqf(T,empty),                                                            \
        __Adqf(T,iter_head),                                                        \
        __Adqf(T,iter_tail),                                                        \
        __Adqf(T,iter_next),                                                        \
        __Adqf(T,iter_prev),                                                        \
    };                                                                              \

#define ADeque_Generate(T)                                                          \
    static inline T* __Adqf(T,at)(const ADeque(T)* self, uint32_t i){               \
        return __Adeq_at(&self->deq, i);                                            \
    }                                                                               \
                                                                                    \
    static inline void __Adqf(T,pushBack)(ADeque(T)* self, const T obj){            \
        aExcClean();                                                                \
        T objx = A_COPY(T, obj); if(__a_unlikely(aExcOccur())){ return; }           \
        int ret = __Adeq_push_back(&self->deq); uint32_t i = self->deq.num - 1;     \
        if(__a_unlikely(ret != 0)){ aExcSet(ret); A_DEST(T,objx); return; }         \
        *__Adqf(T,at)(self, i) = objx;                                              \
    }                                                                               \
                                                                                    \
    static inline void __Adqf(T,pushFront)(ADeque(T)* self, const T obj){           \
        aExcClean();                                                                \
        T objx = A_COPY(T, obj); if(__a_unlikely(aExcOccur())){ return; }           \
        int ret = __Adeq_push_front(&self->deq); uint32_t i = 0;                    \
        if(__a_unlikely(ret != 0)){ aExcSet(ret); A_DEST(T,objx); return; }         \
        *__Adqf(T,at)(self, i) = objx;                                              \
    }                                                                               \
                                                                                    \
    static inline void __Adqf(T,popBack)(ADeque(T)* self, T* tar){                  \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                               \
        uint32_t i = self->deq.num - 1;                                             \
        if(__a_unlikely(self->deq.num == 0)){ aExcSet(AEXC_overstep); return; }     \
        T obj = *((T*)__Adeq_at(&self->deq, i)); __Adeq_pop_back(&self->deq);       \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                         \
    }                                                                               \
                                                                                    \
    static inline void __Adqf(T,popFront)(ADeque(T)* self, T* tar){                 \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                               \
        uint32_t i = 0;                                                             \
        if(__a_unlikely(self->deq.num == 0)){ aExcSet(AEXC_overstep); return; }     \
        T obj = *((T*)__Adeq_at(&self->deq, i)); __Adeq_pop_front(&self->deq);      \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                         \
    }                                                                               \
                                                                                    \
    static inline uint32_t __Adqf(T,getNumber)(const ADeque(T)* self){              \
        return self->deq.num;                                                       \
    }                                                                               \
                                                                                    \
    static inline bool __Adqf(T,empty)(const ADeque(T)* self){                      \
        return self->deq.num == 0;                                                  \
    }                                                                               \
                                                                                    \
    static inline AIter(ADeque(T)) __Adqf(T,iter_head)(const ADeque(T)*self){       \
        AIter(ADeque(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };        \
        if(self->deq.num != 0){                                                     \
            it.p = __Adqf(T,at)(self, 0);                                           \
            it.i = 0;                                                               \
        }                                                                           \
        return it;                                                                  \
    }                                                                               \
                                                                                    \
    static inline AIter(ADeque(T)) __Adqf(T,iter_tail)(const ADeque(T)*self){       \
        AIter(ADeque(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };        \
        if(self->deq.num != 0){                                                     \
            it.p = __Adqf(T,at)(self, self->deq.num - 1);                           \
            it.i = self->deq.num - 1;                                               \
        }                                                                           \
        return it;                                                                  \
    }                                                                               \
                                                                                    \
    static inline void __Adqf(T,iter_next)(AIter(ADeque(T))* it){                   \
        it->i++; it->p = __Adqf(T,at)(it->con, it->i);                              \
    }                                                                               \
                                                                                    \
    static inline void __Adqf(T,iter_prev)(AIter(ADeque(T))* it){                   \
        it->i--; it->p = __Adqf(T,at)(it->con, it->i);                              \
    }                                                                               \
                                                                                    \
    __noused __weak __visibility(protected) void A_OBJ_DEST(ADeque(T))              \
    (ADeque(T)* self){                                                              \
        for(uint32_t i = 0; i < self->deq.num; i++){                                \
            A_DEST(T, *(T*)__Adqf(T,at)(self, i));                                  \
        }                                                                           \
        __Adeq_dest(&self->deq);                                                    \
    }                                                                               \
    __noused __weak __visibility(protected) void A_OBJ_INIT(ADeque(T))              \
    (ADeque(T)* self){                                                              \
        __Adeq_init(&self->deq, sizeof(T));                                         \
        self->f = &A_FUNC_TAB(ADeque(T));                                           \
    }                                                                               \
    __noused __weak __visibility(protected) void A_OBJ_COPY(ADeque(T))              \
    (ADeque(T)* self, const ADeque(T)* that){                                       \
        self->f = that->f;                                                          \
        __Adeq_init(&self->deq, sizeof(T));                                         \
        for(uint32_t i = 0; i < that->deq.num; i++){                                \
            aExcClean();                                                            \
            __Adqf(T,pushBack)(self, *__Adqf(T,at)(that, i));                       \
            if(aExcOccur()) return;                                                 \
        }                                                                           \
    }                                                                               \
    __noused __weak __visibility(protected) int A_OBJ_CMPD(ADeque(T))               \
    (const ADeque(T)*self,const ADeque(T)*that){                                    \
        int ret = 0;                                                                \
        uint32_t num = self->deq.num <= that->deq.num ? self->deq.num:that->deq.num;\
        for(uint32_t i = 0; ret == 0 && i < num; i++){                              \
            ret = A_CMPD(T, *(T*)__Adqf(T,at)(self,i), *(T*)__Adqf(T,at)(that,i));  \
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

#endif /*__adeque_h__*/

