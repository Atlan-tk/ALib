/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __aline_h__
#define __aline_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"


typedef struct{
    char* data;
    uint32_t cap;
    uint32_t num;
    uint32_t size;
    uint32_t offset;
}__Aarr;


static inline void* __Aarr_get_strat(const __Aarr* arr){
    return arr->data + arr->offset * arr->size;
}

void __Aarr_dest(__Aarr* arr);
int __Aarr_copy(__Aarr* arr, const __Aarr* that_arr);
int __Aarr_add_cap_front(__Aarr* arr);
int __Aarr_add_cap_back(__Aarr* arr);
void __Aarr_sub_cap_front(__Aarr* arr);
void __Aarr_sub_cap_back(__Aarr* arr);
void __Aarr_rm(__Aarr* arr, uint32_t index);
int __Aarr_ins(__Aarr* arr, uint32_t index);

static inline int __Aarr_push_front(__Aarr* arr){
    uint32_t red = arr->offset;
    if(__a_unlikely(red == 0 && __Aarr_add_cap_front(arr) != 0)){
        return AEXC_alloc_failed;
    }
    arr->offset--, arr->num++;
    return 0;
}
static inline int __Aarr_push_back(__Aarr* arr){
    uint32_t red = arr->cap - (arr->offset + arr->num);
    if(__a_unlikely(red == 0 && __Aarr_add_cap_back(arr) != 0)){
        return AEXC_alloc_failed;
    }
    arr->num++;
    return 0;
}
static inline void __Aarr_pop_front(__Aarr* arr){
    arr->offset++, arr->num--;
    uint32_t red = arr->offset;
    if(__a_unlikely(arr->cap > 16 && (red * arr->size >= __aPagSize || red >= (arr->cap + 1) / 2))){
        __Aarr_sub_cap_front(arr);
    }
}
static inline void __Aarr_pop_back(__Aarr* arr){
    arr->num--;
    uint32_t red = arr->cap - (arr->offset + arr->num);
    if(__a_unlikely(arr->cap > 16 && (red * arr->size >= __aPagSize || red >= (arr->cap + 1) / 2))){
        __Aarr_sub_cap_back(arr);
    }
}




#define ALine(T) __A_Splice(__A_Generic_Line_$__, T)
#define __Alnf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_Line_$__, T), __func_$__), name)

#define ALine_Define(T)                                                                 \
    typedef struct ALine(T) ALine(T);                                                   \
    typedef struct A_FUNC(ALine(T)) A_FUNC(ALine(T));                                   \
    struct ALine(T){                                                                    \
        const A_FUNC(ALine(T))* f;                                                      \
        __Aarr arr;                                                                     \
        T* p;                                                                           \
        T* type[0];                                                                     \
    };                                                                                  \
                                                                                        \
    AIter_Define(ALine(T));                                                             \
                                                                                        \
    struct A_FUNC(ALine(T)){                                                            \
        bool    flag;                                                                   \
        void    (*dest)(void*);                                                         \
        T*      (*const at)       (const ALine(T)* self, uint32_t index);               \
        void    (*const rm)       (ALine(T)* self, uint32_t index);                     \
        void    (*const ins)      (ALine(T)* self, uint32_t index, const T obj);        \
        void    (*const take)     (ALine(T)* self, uint32_t index, T* tar);             \
        void    (*const pushBack) (ALine(T)* self, const T obj);                        \
        void    (*const pushFront)(ALine(T)* self, const T obj);                        \
        void    (*const popBack)  (ALine(T)* self, T* tat);                             \
        void    (*const popFront) (ALine(T)* self, T* tat);                             \
        uint32_t(*const getNumber)(const ALine(T)* self);                               \
        bool    (*const empty)    (const ALine(T)* self);                               \
        AIter(ALine(T)) (*const head)(const ALine(T)* self);                            \
        AIter(ALine(T)) (*const tail)(const ALine(T)* self);                            \
        void    (*const next)(AIter(ALine(T))* it);                                     \
        void    (*const prev)(AIter(ALine(T))* it);                                     \
    };                                                                                  \
                                                                                        \
    static inline T* __Alnf(T,at)(const ALine(T)* self, uint32_t i);                    \
    static inline void __Alnf(T,rm)(ALine(T)* self, uint32_t i);                        \
    static inline void __Alnf(T,ins)(ALine(T)*self, uint32_t i, const T obj);           \
    static inline void __Alnf(T,take)(ALine(T)* self, uint32_t i, T* tar);              \
    static inline void __Alnf(T,pushBack)(ALine(T)* self, const T obj);                 \
    static inline void __Alnf(T,pushFront)(ALine(T)* self, const T obj);                \
    static inline void __Alnf(T,popBack)(ALine(T)* self, T* tar);                       \
    static inline void __Alnf(T,popFront)(ALine(T)* self, T* tar);                      \
    static inline uint32_t __Alnf(T,getNumber)(const ALine(T)* self);                   \
    static inline bool __Alnf(T,empty)(const ALine(T)* self);                           \
    static inline AIter(ALine(T)) __Alnf(T, iter_head)(const ALine(T)* self);           \
    static inline AIter(ALine(T)) __Alnf(T, iter_tail)(const ALine(T)* self);           \
    static inline void __Alnf(T, iter_next)(AIter(ALine(T))* it);                       \
    static inline void __Alnf(T, iter_prev)(AIter(ALine(T))* it);                       \
    static inline void __A_OBJ_DEST_FUNC_SELF(ALine(T))(ALine(T)*);                     \
    static inline void __A_OBJ_INIT_FUNC_SELF(ALine(T))(ALine(T)*);                     \
    static inline void __A_OBJ_COPY_FUNC_SELF(ALine(T))(ALine(T)*, const ALine(T)*);    \
    static inline int __A_OBJ_CMPD_FUNC_SELF(ALine(T))(const ALine(T)*,const ALine(T)*);\
                                                                                        \
    static const A_FUNC(ALine(T)) A_FUNC_TAB(ALine(T)) = {                              \
        true,                                                                           \
        (void*)__A_OBJ_DEST_FUNC_SELF(ALine(T)),                                        \
        __Alnf(T,at),                                                                   \
        __Alnf(T,rm),                                                                   \
        __Alnf(T,ins),                                                                  \
        __Alnf(T,take),                                                                 \
        __Alnf(T,pushBack),                                                             \
        __Alnf(T,pushFront),                                                            \
        __Alnf(T,popBack),                                                              \
        __Alnf(T,popFront),                                                             \
        __Alnf(T,getNumber),                                                            \
        __Alnf(T,empty),                                                                \
        __Alnf(T,iter_head),                                                            \
        __Alnf(T,iter_tail),                                                            \
        __Alnf(T,iter_next),                                                            \
        __Alnf(T,iter_prev),                                                            \
    };                                                                                  \

#define ALine_Generate(T)                                                               \
    static inline T* __Alnf(T,at)(const ALine(T)* self, uint32_t i){                    \
        if(__a_unlikely(self->arr.num == 0)){ aExcSet(AEXC_overstep); return nullptr; } \
        if(__a_unlikely(i >= self->arr.num)) i = self->arr.num - 1 ;                    \
        return &(self->p[i]);                                                           \
    }                                                                                   \
                                                                                        \
    static inline void __Alnf(T,rm)(ALine(T)* self, uint32_t i){                        \
        if(__a_unlikely(self->arr.num == 0)) return;                                    \
        if(i >= self->arr.num){ i = self->arr.num - 1; } A_DEST(T, self->p[i]);         \
        __Aarr_rm(&self->arr, i); self->p = __Aarr_get_strat(&self->arr);               \
    }                                                                                   \
                                                                                        \
    static inline void __Alnf(T,ins)(ALine(T)*self, uint32_t i, const T obj){           \
        if(i > self->arr.num) i = self->arr.num ;                                       \
        aExcClean(); T objx = A_COPY(T, obj); if(aExcOccur()){ return; }                \
        int ret = __Aarr_ins(&self->arr, i); self->p = __Aarr_get_strat(&self->arr);    \
        if(__a_unlikely(ret != 0)){ aExcSet(ret); A_DEST(T, objx); return; }            \
        self->p[i] = objx;                                                              \
    }                                                                                   \
                                                                                        \
    static inline void __Alnf(T,take)(ALine(T)* self, uint32_t i, T* tar){              \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                                   \
        if(__a_unlikely(self->arr.num == 0)){ aExcSet(AEXC_overstep); return; }         \
        if(i >= self->arr.num){ i = self->arr.num - 1; } T obj = self->p[i];            \
        __Aarr_rm(&self->arr, i);self->p = __Aarr_get_strat(&self->arr);                \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                             \
    }                                                                                   \
                                                                                        \
    static inline void __Alnf(T,pushBack)(ALine(T)* self, const T obj){                 \
        aExcClean();                                                                    \
        uint32_t i = self->arr.num; T objx = A_COPY(T,obj);if(aExcOccur())return;       \
        int ret = __Aarr_push_back(&self->arr); self->p = __Aarr_get_strat(&self->arr); \
        if(__a_unlikely(ret != 0)){ aExcSet(ret); A_DEST(T, objx); return; }            \
        self->p[i] = objx;                                                              \
    }                                                                                   \
                                                                                        \
    static inline void __Alnf(T,pushFront)(ALine(T)* self, const T obj){                \
        aExcClean();                                                                    \
        uint32_t i = 0; T objx = A_COPY(T, obj); if(aExcOccur()) return;                \
        int ret = __Aarr_push_front(&self->arr); self->p = __Aarr_get_strat(&self->arr);\
        if(__a_unlikely(ret != 0)){ aExcSet(ret); A_DEST(T, objx); return; }            \
        self->p[i] = objx;                                                              \
    }                                                                                   \
                                                                                        \
    static inline void __Alnf(T,popBack)(ALine(T)* self, T* tar){                       \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                                   \
        if(__a_unlikely(self->arr.num == 0)){ aExcSet(AEXC_overstep); return; }         \
        uint32_t i = self->arr.num - 1; T obj = self->p[i];                             \
        __Aarr_pop_back(&self->arr); self->p = __Aarr_get_strat(&self->arr);            \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                             \
    }                                                                                   \
                                                                                        \
    static inline void __Alnf(T,popFront)(ALine(T)* self, T* tar){                      \
        if(tar != nullptr) memset(tar, 0, sizeof(T));                                   \
        if(__a_unlikely(self->arr.num == 0)){ aExcSet(AEXC_overstep); return; }         \
        uint32_t i = 0; T obj = self->p[i];                                             \
        __Aarr_pop_front(&self->arr); self->p = __Aarr_get_strat(&self->arr);           \
        if(tar != nullptr) *tar = obj; else A_DEST(T, obj);                             \
    }                                                                                   \
                                                                                        \
    static inline uint32_t __Alnf(T,getNumber)(const ALine(T)* self){                   \
        return self->arr.num;                                                           \
    }                                                                                   \
                                                                                        \
    static inline bool __Alnf(T,empty)(const ALine(T)* self){                           \
        return self->arr.num == 0;                                                      \
    }                                                                                   \
                                                                                        \
    static inline AIter(ALine(T)) __Alnf(T, iter_head)(const ALine(T)* self){           \
        AIter(ALine(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };             \
        if(self->arr.num != 0){                                                         \
            it.p = self->p, it.i = 0;                                                   \
        }                                                                               \
        return it;                                                                      \
    }                                                                                   \
    static inline AIter(ALine(T)) __Alnf(T, iter_tail)(const ALine(T)* self){           \
        AIter(ALine(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };             \
        if(self->arr.num != 0){                                                         \
            it.p = self->p + self->arr.num - 1, it.i = self->arr.num - 1;               \
        }                                                                               \
        return it;                                                                      \
    }                                                                                   \
    static inline void __Alnf(T, iter_next)(AIter(ALine(T))* it){                       \
        it->p++, it->i++;                                                               \
    }                                                                                   \
    static inline void __Alnf(T, iter_prev)(AIter(ALine(T))* it){                       \
        it->p--, it->i--;                                                               \
    }                                                                                   \
                                                                                        \
    __noused __weak void A_OBJ_DEST(ALine(T))(ALine(T)* self){                          \
        for(uint32_t i = 0; i < self->arr.num; i++){                                    \
            A_DEST(T, self->p[i]);                                                      \
        }                                                                               \
        __Aarr_dest(&self->arr);                                                        \
    }                                                                                   \
    __noused __weak void A_OBJ_INIT(ALine(T))(ALine(T)* self){                          \
        self->arr.size = sizeof(T); self->f = &A_FUNC_TAB(ALine(T));                    \
    }                                                                                   \
    __noused __weak void A_OBJ_COPY(ALine(T))(ALine(T)* self, const ALine(T)* that){    \
        self->f = that->f;                                                              \
        int ret = __Aarr_copy(&self->arr, &that->arr);                                  \
        self->p = __Aarr_get_strat(&self->arr);                                         \
        if(ret != 0){ aExcSet(AEXC_alloc_failed); return; }                             \
        for(uint32_t i = 0; i < that->arr.num; i++){                                    \
            aExcClean(); self->p[i] = A_COPY(T, that->p[i]); if(aExcOccur()){ return; } \
            self->arr.num++;                                                            \
        }                                                                               \
    }                                                                                   \
    __noused __weak int  A_OBJ_CMPD(ALine(T))(const ALine(T)* self,const ALine(T)*that){\
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

#endif /*__aline_h__*/

