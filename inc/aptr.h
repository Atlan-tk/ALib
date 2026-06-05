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

#define __Apf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_ptr_$__, T), __func_$__), name)

#define APtr_Define(T)                                                                  \
    typedef struct APtr(T) APtr(T);                                                     \
    struct APtr(T){                                                                     \
        T* p;                                                                           \
        void(*dest)(void*);                                                             \
        bool strong_flag;                                                               \
    };                                                                                  \

#define APtr_Generate(T)                                                                \
    __noused static inline void A_OBJ_DEST(APtr(T))(APtr(T)* self){                     \
        if(self->strong_flag && self->p != nullptr){                                    \
            alib_delete(self->p, self->dest);                                           \
            self->p = nullptr, self->strong_flag = false;                               \
        }                                                                               \
    }                                                                                   \
    __noused static inline void A_OBJ_INIT(APtr(T))(APtr(T)* self){                     \
        self->p = nullptr, self->strong_flag = false;                                   \
        self->dest = (__A_IS_CLASS(T) != 0) ?                                           \
            (void*)a_class_dest : (void*)__A_OBJ_DEST_FUNC_SELF(T);                     \
    }                                                                                   \
    __noused static inline void A_OBJ_COPY(APtr(T))                                     \
    (APtr(T)* self, const APtr(T)* that){                                               \
        self->p = that->p, self->strong_flag = false; self->dest = that->dest;          \
    }                                                                                   \
    __noused static inline int  A_OBJ_CMPD(APtr(T))                                     \
    (const APtr(T)* self,const APtr(T)*that){                                           \
        if(self->p == that->p) return 0;                                                \
        if(self->p == nullptr) return -1;                                               \
        if(that->p == nullptr) return 1;                                                \
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \
    __noused static inline APtr(T) __Apf(T,newobj)(void){                               \
        APtr(T) ptr; A_OBJ_INIT(APtr(T))(&ptr);                                         \
        ptr.p = A_NEW(T); if(ptr.p != nullptr) ptr.strong_flag = true;                  \
        return ptr;                                                                     \
    }                                                                                   \
    __noused static inline APtr(T) __Apf(T,cpnewobj)(T thatobj){                        \
        APtr(T) ptr; A_OBJ_INIT(APtr(T))(&ptr);                                         \
        ptr.p = A_CPNEW(T, thatobj); if(ptr.p != nullptr) ptr.strong_flag = true;       \
        return ptr;                                                                     \
    }                                                                                   \


#define APtrNew(T) __Apf(T, newobj)()

#define APtrCPNew(T, obj) __Apf(T, cpnewobj)((obj))

#define APtrCvs(T, ap) A_MOVE(*(APtr(T)*)&(ap))



/* 初始化计数为1 */
__noused static inline void __a_ref_count_init(atomic_uint* ref_count){
    atomic_store_explicit(ref_count, 1, memory_order_relaxed);
}
__noused static inline bool __a_ref_count_valid(atomic_uint* ref_count){
    return atomic_load_explicit(ref_count, memory_order_relaxed) >= 1;
}
/* 返回为真则自增成功 */
static inline bool __a_ref_count_add(atomic_uint* ref_count){
    if(__a_unlikely(atomic_fetch_add(ref_count, 1) > 0)){
        return true;
    }else{
        atomic_store(ref_count, 0);
    }
    return false;
}
/* 返回为真则可释放 */
static inline bool __a_ref_count_sub(atomic_uint* ref_count){
    if(__a_unlikely(atomic_fetch_sub(ref_count, 1) == 1)){
        return true;
    }
    return false;
}




#define AShPtr(T) __A_Splice(__A_Generic_ShPtr_$__, T)

#define __Aspf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_shptr_$__, T), __func_$__), name)

#define __AShPtr_cpnew(T) __A_Splice(__A_Splice(AShPtr(T), __func_$__), cpnew)

#define AShPtr_Define(T)                                                                \
    typedef struct AShPtr(T) AShPtr(T);                                                 \
    struct AShPtr(T){                                                                   \
        T* p;                                                                           \
        void(*dest)(void*);                                                             \
        atomic_uint* ref_count;                                                         \
    };                                                                                  \

#define AShPtr_Generate(T)                                                              \
    __noused static inline void A_OBJ_DEST(AShPtr(T))(AShPtr(T)* self){                 \
        if(__a_likely(self->p != nullptr)){                                             \
            if(__a_unlikely(__a_ref_count_sub(self->ref_count))){                       \
                alib_delete(self->p, self->dest);                                       \
                alib_free(self->ref_count);                                             \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    __noused static inline void A_OBJ_INIT(AShPtr(T))(AShPtr(T)* self){                 \
        self->p = nullptr, self->ref_count = nullptr;                                   \
        self->dest = (__A_IS_CLASS(T) != 0) ?                                           \
            (void*)a_class_dest : (void*)__A_OBJ_DEST_FUNC_SELF(T);                     \
    }                                                                                   \
    __noused static inline void A_OBJ_COPY(AShPtr(T))                                   \
    (AShPtr(T)* self, const AShPtr(T)* that){                                           \
        self->dest = that->dest;                                                        \
        if(__a_likely(that->p != nullptr)){                                             \
            if(__a_likely(__a_ref_count_add(that->ref_count))){                         \
                *self = *that;                                                          \
            }else{                                                                      \
                aExcSet(AEXC_init_failed);                                              \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    __noused static inline int  A_OBJ_CMPD(AShPtr(T))                                   \
    (const AShPtr(T)* self,const AShPtr(T)*that){                                       \
        if(self->p == that->p) return 0;                                                \
        if(self->p == nullptr) return -1;                                               \
        if(that->p == nullptr) return 1;                                                \
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \
    __noused static inline AShPtr(T) __Aspf(T, newobj)(void){                           \
        AShPtr(T) ptr; A_OBJ_INIT(AShPtr(T))(&ptr);                                     \
        ptr.ref_count = alib_alloc(sizeof(atomic_uint));                                \
        if(__a_unlikely(ptr.ref_count == nullptr)){                                     \
            aExcSet(AEXC_alloc_failed); return ptr;                                     \
        }                                                                               \
        ptr.p = A_NEW(T); if(__a_unlikely(ptr.p == nullptr)){                           \
            alib_free(ptr.ref_count);                                                   \
            memset(&ptr, 0, sizeof(AShPtr(T))); return ptr;                             \
        }                                                                               \
        __a_ref_count_init(ptr.ref_count);                                              \
        return ptr;                                                                     \
    }                                                                                   \
    __noused static inline AShPtr(T) __Aspf(T, cpnewobj)(T obj){                        \
        AShPtr(T) ptr; A_OBJ_INIT(AShPtr(T))(&ptr);                                     \
        ptr.ref_count = alib_alloc(sizeof(atomic_uint));                                \
        if(__a_unlikely(ptr.ref_count == nullptr)){                                     \
            aExcSet(AEXC_alloc_failed); return ptr;                                     \
        }                                                                               \
        ptr.p = A_CPNEW(T, obj); if(__a_unlikely(ptr.p == nullptr)){                    \
            alib_free(ptr.ref_count);                                                   \
            memset(&ptr, 0, sizeof(AShPtr(T))); return ptr;                             \
        }                                                                               \
        __a_ref_count_init(ptr.ref_count);                                              \
        return ptr;                                                                     \
    }                                                                                   \

#define AShPtrNew(T) __Aspf(T, newobj)()

#define AShPtrCPNew(T, obj) __Aspf(T, cpnewobj)((obj))

#define AShPtrCvs(T, ap) A_MOVE(*(AShPtr(T)*)&(ap))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__aptr_h__*/
