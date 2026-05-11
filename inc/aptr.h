/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __aptr_h__
#define __aptr_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include <stdatomic.h>


#define APtr(T) __A_Splice(__A_Generic_Ptr_$__, T)

#define APtr_Define(T)                                                                  \
    typedef struct APtr(T) APtr(T);                                                     \
    struct APtr(T){                                                                     \
        T* p;                                                                           \
        bool strong_flag;                                                               \
    };                                                                                  \

#define APtr_Generate(T)                                                                \
    __unused static void A_OBJ_DEST(APtr(T))(APtr(T)* self){                            \
        if(self->strong_flag && self->p != nullptr)                                     \
            A_DELETE(T, self->p),self->p = nullptr, self->strong_flag = false;          \
    }                                                                                   \
    __unused static void A_OBJ_INIT(APtr(T))(APtr(T)* self){                            \
        self->p = A_NEW(T); if(self->p != nullptr) self->strong_flag = true;            \
    }                                                                                   \
    __unused static void A_OBJ_COPY(APtr(T))(APtr(T)* self, const APtr(T)* that){       \
        self->p = that->p, self->strong_flag = false;                                   \
    }                                                                                   \
    __unused static int  A_OBJ_CMPD(APtr(T))(const APtr(T)* self,const APtr(T)*that){   \
        if(self->p == that->p) return 0;                                                \
        if(self->p == nullptr) return -1;                                               \
        if(that->p == nullptr) return 1;                                                \
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \

#define APtrNew(T) A_INIT(APtr(T))                                                      \

#define APtrCPNew(T, obj) ({                                                            \
    auto __a_obj = (obj); __a_type_assert(T, __a_obj);                                  \
    (APtr(T)){ A_CPNEW(T, __a_obj), true };                                             \
})                                                                                      \



/* 初始化计数为1 */
static inline void __a_ref_count_init(atomic_int* ref_count){
    atomic_store_explicit(ref_count, 1, memory_order_relaxed);
}
__unused static inline bool __a_ref_count_valid(atomic_int* ref_count){
    return atomic_load_explicit(ref_count, memory_order_relaxed) >= 1;
}
/* 返回为真则自增成功 */
static inline bool __a_ref_count_add(atomic_int* ref_count){
    if(__a_unlikely(atomic_fetch_add(ref_count, 1) > 0)){
        return true;
    }else{
        atomic_store(ref_count, 0);
    }
    return false;
}
/* 返回为真则可释放 */
static inline bool __a_ref_count_sub(atomic_int* ref_count){
    if(__a_unlikely(atomic_fetch_sub(ref_count, 1) == 1)){
        return true;
    }
    return false;
}




#define AShPtr(T) __A_Splice(__A_Generic_ShPtr_$__, T)

#define __Asp_tar(T) __A_Splice(AShPtr(T), _$_tarStruct__)

#define __Aspf(T, name) __A_Splice(__A_Splice(AShPtr(T), __func_$__), name)

#define AShPtr_Define(T)                                                                \
    typedef struct AShPtr(T) AShPtr(T);                                                 \
    struct AShPtr(T){                                                                   \
        T* p;                                                                           \
    };                                                                                  \

#define AShPtr_Generate(T)                                                              \
    typedef struct { atomic_int ref_count; T data; } __Asp_tar(T);                      \
    static inline void __Aspf(T, data_init)(__Asp_tar(T)* tar){                         \
        memset(tar, 0, sizeof(__Asp_tar(T)));                                           \
        aExcClean(); tar->data = A_INIT(T);                                             \
        if(!aExcOccur()) __a_ref_count_init(&(tar->ref_count));                         \
    }                                                                                   \
    static inline void __Aspf(T, data_dest)(__Asp_tar(T)* tar){                         \
        A_DEST(T, tar->data); memset(tar, 0, sizeof(__Asp_tar(T)));                     \
    }                                                                                   \
    static inline void __Aspf(T, data_copy)(__Asp_tar(T)* tar, const __Asp_tar(T)*that){\
        memset(tar, 0, sizeof(__Asp_tar(T)));                                           \
        aExcClean(); tar->data = A_COPY(T, that->data);                                 \
        if(!aExcOccur()) __a_ref_count_init(&(tar->ref_count));                         \
    }                                                                                   \
                                                                                        \
    __unused static void A_OBJ_DEST(AShPtr(T))(AShPtr(T)* self){                        \
        if(__a_likely(self->p != nullptr)){                                             \
            __Asp_tar(T)* tar = container_of(self->p, __Asp_tar(T), data);              \
            if(__a_unlikely(__a_ref_count_sub(&(tar->ref_count)))){                     \
                alib_delete(tar, (void*)__Aspf(T,data_dest));                           \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    __unused static void A_OBJ_INIT(AShPtr(T))(AShPtr(T)* self){                        \
        __Asp_tar(T)* p=alib_new(sizeof(__Asp_tar(T)),(void*)__Aspf(T,data_init));      \
        if(p != nullptr) self->p = &(p->data);                                          \
    }                                                                                   \
    __unused static void A_OBJ_COPY(AShPtr(T))(AShPtr(T)* self, const AShPtr(T)* that){ \
        if(__a_likely(that->p != nullptr)){                                             \
            __Asp_tar(T)* tar = container_of(that->p, __Asp_tar(T), data);              \
            if(__a_likely(__a_ref_count_add(&(tar->ref_count)))){                       \
                self->p = that->p;                                                      \
            }else{                                                                      \
                aExcSet(AEXC_init_failed);                                              \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    __unused static int  A_OBJ_CMPD(AShPtr(T))(const AShPtr(T)* self,const AShPtr(T)*that){   \
        if(self->p == that->p) return 0;                                                \
        if(self->p == nullptr) return -1;                                               \
        if(that->p == nullptr) return 1;                                                \
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \

#define AShPtrNew(T) A_INIT(AShPtr(T))                                                  \

#define AShPtrCPNew(T, obj) ({                                                          \
    auto __a_obj = (obj); __a_type_assert(T, __a_obj);                                  \
    __Asp_tar(T) __a_tar = { 0, __a_obj };                                              \
    __Asp_tar(T)*__a_p = alib_cpnew(sizeof(__Asp_tar(T)),                               \
            &__a_tar, (void*)__Aspf(T,data_copy));                                      \
    (AShPtr(T)){ __a_p != nullptr ? & (__a_p->data) : nullptr };                        \
})                                                                                      \



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__aptr_h__*/

