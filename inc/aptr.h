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
        void(*dest)(void*);                                                             \
        bool strong_flag;                                                               \
    };                                                                                  \

#define APtr_Generate(T)                                                                \
    __noused __weak void A_OBJ_DEST(APtr(T))(APtr(T)* self){                            \
        if(self->strong_flag && self->p != nullptr){                                    \
            alib_delete(self->p, self->dest);                                           \
            self->p = nullptr, self->strong_flag = false;                               \
        }                                                                               \
    }                                                                                   \
    __noused __weak void A_OBJ_INIT(APtr(T))(APtr(T)* self){                            \
        self->p = A_NEW(T); if(self->p != nullptr) self->strong_flag = true;            \
        self->dest = (__A_IS_CLASS(T) != 0) ?                                           \
            (void*)a_class_dest : (void*)__A_OBJ_DEST_FUNC_SELF(T);                     \
    }                                                                                   \
    __noused __weak void A_OBJ_COPY(APtr(T))(APtr(T)* self, const APtr(T)* that){       \
        self->p = that->p, self->strong_flag = false; self->dest = that->dest;          \
    }                                                                                   \
    __noused __weak int  A_OBJ_CMPD(APtr(T))(const APtr(T)* self,const APtr(T)*that){   \
        if(self->p == that->p) return 0;                                                \
        if(self->p == nullptr) return -1;                                               \
        if(that->p == nullptr) return 1;                                                \
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \

#define APtrNew(T) A_INIT(APtr(T))                                                      \

#define APtrCPNew(T, obj) ({                                                            \
    auto __a_aptr_obj = (obj); __a_type_assert(T, __a_aptr_obj);                        \
    (APtr(T)){ .p = A_CPNEW(T, __a_aptr_obj), .strong_flag = true,                      \
        .dest = (__A_IS_CLASS(T) != 0) ?                                                \
            (void*)a_class_dest : (void*)__A_OBJ_DEST_FUNC_SELF(T)                      \
    };                                                                                  \
})                                                                                      \

#define APtrCvs(T, ap)({                                                                \
    auto __a_aptr = &(ap); APtr(T) __a_aptr_new;                                        \
    memcpy(&__a_aptr_new, (const void*)__a_aptr, sizeof(APtr(T)));                      \
    memset(__a_aptr, 0, sizeof(APtr(T)));                                               \
    __a_aptr_new;                                                                       \
})                                                                                      \



/* 初始化计数为1 */
static inline void __a_ref_count_init(atomic_ullong* ref_count){
    atomic_store_explicit(ref_count, 1, memory_order_relaxed);
}
__noused static inline bool __a_ref_count_valid(atomic_ullong* ref_count){
    return atomic_load_explicit(ref_count, memory_order_relaxed) >= 1;
}
/* 返回为真则自增成功 */
static inline bool __a_ref_count_add(atomic_ullong* ref_count){
    if(__a_unlikely(atomic_fetch_add(ref_count, 1) > 0)){
        return true;
    }else{
        atomic_store(ref_count, 0);
    }
    return false;
}
/* 返回为真则可释放 */
static inline bool __a_ref_count_sub(atomic_ullong* ref_count){
    if(__a_unlikely(atomic_fetch_sub(ref_count, 1) == 1)){
        return true;
    }
    return false;
}




#define AShPtr(T) __A_Splice(__A_Generic_ShPtr_$__, T)

#define __AShPtr_cpnew(T) __A_Splice(__A_Splice(AShPtr(T), __func_$__), cpnew)

#define AShPtr_Define(T)                                                                \
    typedef struct AShPtr(T) AShPtr(T);                                                 \
    struct AShPtr(T){                                                                   \
        T* p;                                                                           \
        void(*dest)(void*);                                                             \
        atomic_ullong* ref_count;                                                       \
    };                                                                                  \

#define AShPtr_Generate(T)                                                              \
    __noused __weak void A_OBJ_DEST(AShPtr(T))(AShPtr(T)* self){                        \
        if(__a_likely(self->p != nullptr)){                                             \
            if(__a_unlikely(__a_ref_count_sub(self->ref_count))){                       \
                alib_delete(self->p, self->dest);                                       \
                alib_free(self->ref_count);                                             \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    __noused __weak void A_OBJ_INIT(AShPtr(T))(AShPtr(T)* self){                        \
        self->ref_count = alib_alloc(sizeof(atomic_ullong));                            \
        if(__a_unlikely(self->ref_count == nullptr)){                                   \
            aExcSet(AEXC_alloc_failed); return;                                         \
        }                                                                               \
        self->p = A_NEW(T); if(__a_unlikely(self->p == nullptr)){                       \
            alib_free(self->ref_count);                                                 \
            memset(self, 0, sizeof(AShPtr(T))); return;                                 \
        }                                                                               \
        atomic_store_explicit(self->ref_count, 1, memory_order_relaxed);                \
        self->dest = (__A_IS_CLASS(T) != 0) ?                                           \
            (void*)a_class_dest : (void*)__A_OBJ_DEST_FUNC_SELF(T);                     \
    }                                                                                   \
    __noused __weak void A_OBJ_COPY(AShPtr(T))(AShPtr(T)* self, const AShPtr(T)* that){ \
        if(__a_likely(that->p != nullptr)){                                             \
            if(__a_likely(__a_ref_count_add(that->ref_count))){                         \
                *self = *that;                                                          \
            }else{                                                                      \
                aExcSet(AEXC_init_failed);                                              \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    __noused __weak int  A_OBJ_CMPD(AShPtr(T))(const AShPtr(T)* self,const AShPtr(T)*that){   \
        if(self->p == that->p) return 0;                                                \
        if(self->p == nullptr) return -1;                                               \
        if(that->p == nullptr) return 1;                                                \
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \
    __noused static inline AShPtr(T) __AShPtr_cpnew(T)(T* obj){                         \
        AShPtr(T) sh = {};                                                              \
        sh.ref_count = alib_alloc(sizeof(atomic_ullong));                               \
        if(__a_unlikely(sh.ref_count == nullptr)){                                      \
            aExcSet(AEXC_alloc_failed); return sh;                                      \
        }                                                                               \
        sh.p = A_CPNEW(T, *obj); if(__a_unlikely(sh.p == nullptr)){                     \
            alib_free(sh.ref_count);                                                    \
            memset(&sh, 0, sizeof(AShPtr(T))); return sh;                               \
        }                                                                               \
        atomic_store_explicit(sh.ref_count, 1, memory_order_relaxed);                   \
        sh.dest = (__A_IS_CLASS(T) != 0) ?                                              \
            (void*)a_class_dest : (void*)__A_OBJ_DEST_FUNC_SELF(T);                     \
        return sh;                                                                      \
    }                                                                                   \

#define AShPtrNew(T) A_INIT(AShPtr(T))                                                  \

#define AShPtrCPNew(T, obj) ({                                                          \
    auto __a_ashptr_obj = (obj); __a_type_assert(T, __a_ashptr_obj);                    \
    __AShPtr_cpnew(T)(&__a_ashptr_obj);                                                 \
})                                                                                      \

#define AShPtrCvs(T, sh)({                                                              \
    auto __a_ashptr = &(sh); AShPtr(T) __a_ashptr_new;                                  \
    memcpy(&__a_ashptr_new, (const void*)__a_ashptr, sizeof(AShPtr(T)));                \
    memset(__a_ashptr, 0, sizeof(AShPtr(T)));                                           \
    __a_ashptr_new;                                                                     \
})                                                                                      \


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__aptr_h__*/
