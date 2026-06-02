/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __ahash_h__
#define __ahash_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"

typedef struct{
    char* p;
    uint32_t num;
    uint32_t cap;
}__AhsBucket;

typedef struct{
    __AhsBucket* bucket_arr;
    uint32_t bucket_num;

    uint32_t num;
    uint32_t size;
    uint32_t size_k;

    void(*dest)(void*);
    void(*copy)(void*,const void*);
    int (*cmpd_v)(const void*,const void*);
    int (*cmpd_k)(const void*,const void*);
    uint32_t (*hash_func)(const void*);
    char type[0];
}__Ahash;

void __Ahash_dest(__Ahash* self);
int  __Ahash_copy(__Ahash* self, const __Ahash* that);
int  __Ahash_cmpd(const __Ahash* self, const __Ahash* that);

void*__Ahash_at(const __Ahash* self, const void* k);
void __Ahash_rm(__Ahash* self, const void* k);
void __Ahash_ins(__Ahash* self, void* data);
void __Ahash_take(__Ahash* self, void* data);

void __Ahash_get_head(const __Ahash* self, __Aiter* it);
void __Ahash_get_tail(const __Ahash* self, __Aiter* it);
void __Ahash_iter_next(const __Ahash* hash, __Aiter* it);
void __Ahash_iter_prev(const __Ahash* hash, __Aiter* it);


#define AHash(TK,TV) __A_3Splice(__A_Generic_Hash_$__, TK, TV)
#define __Ahsf(TK,TV, name) __A_Splice(__A_Splice(AHash(TK,TV), __func_$__), name)
#define __Ahs_data(TK,TV) __A_Splice(AHash(TK,TV), _$__data__)

#define AHash_Define(TK,TV)                                                                     \
    typedef struct AHash(TK,TV) AHash(TK,TV);                                                   \
    typedef struct A_FUNC(AHash(TK,TV)) A_FUNC(AHash(TK,TV));                                   \
    struct AHash(TK,TV){                                                                        \
        const A_FUNC(AHash(TK,TV))* f;                                                          \
        __Ahash hash;                                                                           \
        TV* type[0];                                                                            \
    };                                                                                          \
                                                                                                \
    AIter_Define(AHash(TK,TV));                                                                 \
                                                                                                \
    struct A_FUNC(AHash(TK,TV)){                                                                \
        bool    flag;                                                                           \
        void    (*dest)(void*);                                                                 \
        TV*     (*const at)       (const AHash(TK,TV)* self, const TK k);                       \
        void    (*const rm)       (AHash(TK,TV)* self, const TK k);                             \
        void    (*const ins)      (AHash(TK,TV)* self, const TK k, const TV s);                 \
        void    (*const take)     (AHash(TK,TV)* self, const TK k, TV* target);                 \
        uint32_t(*const getNumber)(const AHash(TK,TV)* self);                                   \
        bool    (*const empty)    (const AHash(TK,TV)* self);                                   \
        AIter(AHash(TK,TV)) (*const head)(const AHash(TK,TV)* self);                            \
        AIter(AHash(TK,TV)) (*const tail)(const AHash(TK,TV)* self);                            \
        void    (*const next)(AIter(AHash(TK,TV))* it);                                         \
        void    (*const prev)(AIter(AHash(TK,TV))* it);                                         \
        TK      (*const getk)(AIter(AHash(TK,TV)) it);                                          \
    };                                                                                          \
                                                                                                \
    static inline TV*      __Ahsf(TK,TV,at)       (const AHash(TK,TV)* self, const TK k);       \
    static inline void     __Ahsf(TK,TV,rm)       (AHash(TK,TV)* self, const TK k);             \
    static inline void     __Ahsf(TK,TV,ins)      (AHash(TK,TV)* self, const TK k, const TV s); \
    static inline void     __Ahsf(TK,TV,take)     (AHash(TK,TV)* self, const TK k, TV* target); \
    static inline uint32_t __Ahsf(TK,TV,getNumber)(const AHash(TK,TV)* self);                   \
    static inline bool     __Ahsf(TK,TV,empty)    (const AHash(TK,TV)* self);                   \
    static inline AIter(AHash(TK,TV)) __Ahsf(TK,TV,iter_head)(const AHash(TK,TV)* self);        \
    static inline AIter(AHash(TK,TV)) __Ahsf(TK,TV,iter_tail)(const AHash(TK,TV)* self);        \
    static inline void     __Ahsf(TK,TV,iter_next)(AIter(AHash(TK,TV))* it);                    \
    static inline void     __Ahsf(TK,TV,iter_prev)(AIter(AHash(TK,TV))* it);                    \
    static inline TK       __Ahsf(TK,TV,iter_getk)(AIter(AHash(TK,TV)) it);                     \
    static inline void __A_OBJ_DEST_FUNC_SELF(AHash(TK,TV))(AHash(TK,TV)*);                     \
    static inline void __A_OBJ_INIT_FUNC_SELF(AHash(TK,TV))(AHash(TK,TV)*);                     \
    static inline void __A_OBJ_COPY_FUNC_SELF(AHash(TK,TV))(AHash(TK,TV)*, const AHash(TK,TV)*);     \
    static inline int __A_OBJ_CMPD_FUNC_SELF(AHash(TK,TV))(const AHash(TK,TV)*,const AHash(TK,TV)*); \
                                                                                                \
    static const A_FUNC(AHash(TK,TV)) A_FUNC_TAB(AHash(TK,TV)) = {                              \
        true,                                                                                   \
        (void*)__A_OBJ_DEST_FUNC_SELF(AHash(TK,TV)),                                            \
        __Ahsf(TK,TV,at),                                                                       \
        __Ahsf(TK,TV,rm),                                                                       \
        __Ahsf(TK,TV,ins),                                                                      \
        __Ahsf(TK,TV,take),                                                                     \
        __Ahsf(TK,TV,getNumber),                                                                \
        __Ahsf(TK,TV,empty),                                                                    \
        __Ahsf(TK,TV,iter_head),                                                                \
        __Ahsf(TK,TV,iter_tail),                                                                \
        __Ahsf(TK,TV,iter_next),                                                                \
        __Ahsf(TK,TV,iter_prev),                                                                \
        __Ahsf(TK,TV,iter_getk),                                                                \
    };                                                                                          \

#define AHash_Generate(TK,TV)                                                                   \
    typedef struct { TK k; TV v; } __Ahs_data(TK,TV);                                           \
    static inline void __Ahsf(TK,TV,data_dest)(__Ahs_data(TK,TV)* data){                        \
        A_DEST(TK, data->k); A_DEST(TV, data->v);                                               \
    }                                                                                           \
    static inline void __Ahsf(TK,TV,data_copy)(__Ahs_data(TK,TV)* data,                         \
            const __Ahs_data(TK,TV)* that_data){                                                \
        data->k = A_COPY(TK, that_data->k);                                                     \
        if(!aExcOccur()){ data->v=A_COPY(TV,that_data->v); }                                    \
        if(aExcOccur()){ A_DEST(TK, data->k); }                                                 \
    }                                                                                           \
    static inline int  __Ahsf(TK,TV,data_cmpd)(const __Ahs_data(TK,TV)* data,                   \
            const __Ahs_data(TK,TV)* that_data){                                                \
        return A_CMPD(TV, data->v, that_data->v);                                               \
    }                                                                                           \
    /******************************************************************************************/\
    static inline TV* __Ahsf(TK,TV,at)(const AHash(TK,TV)* self, const TK k){                   \
        __Ahs_data(TK,TV)* data = __Ahash_at(&self->hash, (const void*)&k);                     \
        if(__a_unlikely(data == nullptr)){ aExcSet(AEXC_overstep); return nullptr; }            \
        return &(data->v);                                                                      \
    }                                                                                           \
    static inline void __Ahsf(TK,TV,rm)(AHash(TK,TV)* self, const TK k){                        \
        __Ahash_rm(&self->hash, (const void*)&k);                                               \
    }                                                                                           \
    static inline void __Ahsf(TK,TV,ins)(AHash(TK,TV)* self, const TK k, const TV v){           \
        aExcClean();                                                                            \
        __Ahs_data(TK,TV) data = {.k = k, .v = v }; __Ahash_ins(&self->hash, &data);            \
    }                                                                                           \
    static inline void __Ahsf(TK,TV,take)(AHash(TK,TV)* self, const TK k, TV* tar){             \
        if(tar != nullptr) memset(tar, 0, sizeof(TV));                                          \
        aExcClean();                                                                            \
        __Ahs_data(TK,TV) data = { .k = k }; __Ahash_take(&self->hash, (void*)&data);           \
        if(__a_unlikely(aExcOccur())){                                                          \
            return;                                                                             \
        }else{                                                                                  \
            A_DEST(TK, data.k); if(tar != nullptr) *tar = data.v; else A_DEST(TV, data.v);      \
        }                                                                                       \
    }                                                                                           \
    static inline uint32_t __Ahsf(TK,TV,getNumber)(const AHash(TK,TV)* self){                   \
        return self->hash.num;                                                                  \
    }                                                                                           \
    static inline bool __Ahsf(TK,TV,empty)(const AHash(TK,TV)* self){                           \
        return self->hash.num == 0;                                                             \
    }                                                                                           \
    static inline AIter(AHash(TK,TV)) __Ahsf(TK,TV,iter_head)(const AHash(TK,TV)* self){        \
        AIter(AHash(TK,TV)) it = { .con = self, .p = nullptr, .i = 0, .r = 0};                  \
        __Ahash_get_head(&self->hash, (void*)&it);                                              \
        it.p = it.p != nullptr ? &((__Ahs_data(TK,TV)*)(it.p))->v : nullptr;                    \
        return it;                                                                              \
    }                                                                                           \
    static inline AIter(AHash(TK,TV)) __Ahsf(TK,TV,iter_tail)(const AHash(TK,TV)* self){        \
        AIter(AHash(TK,TV)) it = { .con = self, .p = nullptr, .i = 0, .r = 0};                  \
        __Ahash_get_tail(&self->hash, (void*)&it);                                              \
        it.p = it.p != nullptr ? &((__Ahs_data(TK,TV)*)(it.p))->v : nullptr;                    \
        return it;                                                                              \
    }                                                                                           \
    static inline void __Ahsf(TK,TV,iter_next)(AIter(AHash(TK,TV))* it){                        \
        AIter(AHash(TK,TV)) the_it = *it;                                                       \
        the_it.p = it->p != nullptr ? (void*)container_of(it->p,__Ahs_data(TK,TV),v) : nullptr; \
        __Ahash_iter_next(&(it->con->hash), (void*)&the_it); *it = the_it;                      \
        it->p = the_it.p != nullptr ? &((__Ahs_data(TK,TV)*)(the_it.p))->v : nullptr;           \
    }                                                                                           \
    static inline void __Ahsf(TK,TV,iter_prev)(AIter(AHash(TK,TV))* it){                        \
        AIter(AHash(TK,TV)) the_it = *it;                                                       \
        the_it.p = it->p != nullptr ? (void*)container_of(it->p,__Ahs_data(TK,TV),v) : nullptr; \
        __Ahash_iter_prev(&(it->con->hash), (void*)&the_it); *it = the_it;                      \
        it->p = the_it.p != nullptr ? &((__Ahs_data(TK,TV)*)(the_it.p))->v : nullptr;           \
    }                                                                                           \
    static inline TK __Ahsf(TK,TV,iter_getk)(AIter(AHash(TK,TV)) it){                           \
        TK k; memset(&k, 0, sizeof(TK));                                                        \
        if(__a_unlikely(it.p == nullptr)){ aExcSet(AEXC_overstep); return k; }                  \
        return container_of(it.p,__Ahs_data(TK,TV),v)->k;                                       \
    }                                                                                           \
                                                                                                \
    uint32_t A_OBJ_HASH(TK)(const TK* self);                                                    \
    __weakref(A_OBJ_HASH(TK)) static uint32_t __A_OBJ_HASH(TK)(const TK* self);                 \
    __noused static inline void A_OBJ_INIT(AHash(TK,TV))(AHash(TK,TV)* self){                   \
        self->f = &A_FUNC_TAB(AHash(TK,TV));                                                    \
        self->hash.size = sizeof(__Ahs_data(TK,TV));                                            \
        self->hash.size_k = sizeof(TK);                                                         \
        self->hash.hash_func = (void*)__A_OBJ_HASH(TK);                                         \
        self->hash.copy = (void*)__Ahsf(TK,TV,data_copy);                                       \
        self->hash.dest = (void*)__Ahsf(TK,TV,data_dest);                                       \
        self->hash.cmpd_v = (void*)__Ahsf(TK,TV,data_cmpd);                                     \
        self->hash.cmpd_k = (void*)__A_OBJ_CMPD_FUNC_SELF(TK);                                  \
    }                                                                                           \
    __noused static inline void A_OBJ_DEST(AHash(TK,TV))(AHash(TK,TV)* self){                   \
        __Ahash_dest(&self->hash);                                                              \
    }                                                                                           \
    __noused static inline void A_OBJ_COPY(AHash(TK,TV))                                        \
    (AHash(TK,TV)* self, const AHash(TK,TV)* that){                                             \
        self->f = that->f;                                                                      \
        int ret = __Ahash_copy(&self->hash, &that->hash);                                       \
        if(__a_unlikely(ret != 0)) aExcSet(ret);                                                \
    }                                                                                           \
    __noused static inline int  A_OBJ_CMPD(AHash(TK,TV))                                        \
    (const AHash(TK,TV)*self,const AHash(TK,TV)*that){                                          \
        return __Ahash_cmpd(&self->hash, &that->hash);                                          \
    }                                                                                           \

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__ahash_h__*/

