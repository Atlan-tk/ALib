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
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \



#define AShPtr(T) __A_Splice(__A_Generic_ShPtr_$__, T)

#define AShPtr_Define(T)                                                                \
    typedef struct AShPtr(T) AShPtr(T);                                                 \
    struct AShPtr(T){                                                                   \
        T* p;                                                                           \
    };                                                                                  \

#define AShPtr_Generate(T)                                                              \
    __unused static void A_OBJ_DEST(AShPtr(T))(AShPtr(T)* self){                        \
        if(self->p != nullptr){                                                         \
            alib_ref_delete(self->p, (void*)__A_OBJ_DEST_FUNC_SELF(T));                 \
            self->p = nullptr;                                                          \
        }                                                                               \
    }                                                                                   \
    __unused static void A_OBJ_INIT(AShPtr(T))(AShPtr(T)* self){                        \
        self->p = alib_ref_new(sizeof(T), (void*)__A_OBJ_INIT_FUNC_SELF(T));            \
    }                                                                                   \
    __unused static void A_OBJ_COPY(AShPtr(T))(AShPtr(T)* self, const AShPtr(T)* that){ \
        self->p = alib_ref_copy(that->p);                                               \
    }                                                                                   \
    __unused static int  A_OBJ_CMPD(AShPtr(T))(const AShPtr(T)* self,const AShPtr(T)*that){   \
        return A_CMPD(T, *self->p, *that->p);                                           \
    }                                                                                   \


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__aline_h__*/

