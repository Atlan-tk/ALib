/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __aclass_h__
#define __aclass_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"

/* class system */
/********************************************************************/
#define AClass_Inherit(T, ...)                                                                              \
    static_assert(__aNarg_n(0, ##__VA_ARGS__) <= 1, "Error: Only single inheritance is allowed");           \
    typedef struct T T;                                                                                     \
    typedef struct A_FUNC(T) A_FUNC(T);                                                                     \
    __AClass_Inherit(T, ##__VA_ARGS__, Atlan);                                                              \
                                                                                                            \

#define AClass_Struct(T, ...)                                                                               \
    struct T{                                                                                               \
        union{                                                                                              \
            const A_FUNC(T)* f;                                                                             \
            __A_CLASS_BASE(T) __base__;                                                                     \
        };                                                                                                  \
        __VA_ARGS__;                                                                                        \
    };                                                                                                      \

#define AClass_Function(T, ...)                                                                             \
    struct A_FUNC(T){                                                                                       \
        union{                                                                                              \
            bool flag;                                                                                      \
            __A_FUNC_BASE(T) __base__;                                                                      \
        };                                                                                                  \
        __VA_ARGS__;                                                                                        \
    };                                                                                                      \

#define AClass_Generate(T, ...)                                                                             \
    static A_FUNC(T) A_FUNC_TAB(T) = { { false }, __VA_ARGS__ };                                            \

#define A_CLASS_REGISTER(T)                                                                                 \
    enum{ __A_IS_CLASS(T) = 1 };                                                                            \
                                                                                                            \
    __weak void A_SET_VTAB(T)(T* self);                                                                     \
    __weak void A_OBJ_DEST(T)(T* self);                                                                     \
    __weak void A_OBJ_INIT(T)(T* self);                                                                     \
    __weak void A_OBJ_COPY(T)(T* self, const T* that);                                                      \
    __weak int  A_OBJ_CMPD(T)(const T* self, const T* that);                                                \
    __weak uint32_t A_OBJ_HASH(T)(const T*);                                                                \
                                                                                                            \
    static inline void __A_OBJ_INIT_FUNC_SELF(T)(T* self);                                                  \
    static inline void __A_OBJ_DEST_FUNC_SELF(T)(T* self);                                                  \
    static inline void __A_OBJ_COPY_FUNC_SELF(T)(T* self, const T* that);                                   \
    static inline int  __A_OBJ_CMPD_FUNC_SELF(T)(const T* self, const T* that);                             \
                                                                                                            \
    __noused static inline void __A_OBJ_DEST_FUNC_SELF(T)(T* self){                                         \
        /* 空对象不析构 */                                                                                  \
        if(__a_unlikely(self == nullptr)) { return; }                                                       \
                                                                                                            \
        T null_obj; memset(&null_obj, 0, sizeof(T));                                                        \
        if(memcmp(self, &null_obj, sizeof(T)) == 0){ return; }                                              \
                                                                                                            \
        if(A_OBJ_DEST(T) != nullptr) A_OBJ_DEST(T)(self);                                                   \
        __A_OBJ_DEST_FUNC_BASE(T)((__A_CLASS_BASE(T)*)self);                                                \
        memset(self, 0, sizeof(T));                                                                         \
    }                                                                                                       \
    __noused static inline void __A_OBJ_INIT_FUNC_SELF(T)(T* self){                                         \
        aExcClean();                                                                                        \
        if(__a_unlikely(self == nullptr)) { aExcSet(AEXC_nullptr); return; }                                \
                                                                                                            \
        A_FUNC(T)* f = (void*)&A_FUNC_TAB(T); __A_FUNC_BASE(T)* bf = (void*)__A_FUNC_TAB_BASE(T);           \
        memset(self, 0, sizeof(T));  auto flag = f->flag; self->f = f;                                      \
        __A_OBJ_INIT_FUNC_BASE(T)((void*)self); self->f = f;                                                \
                                                                                                            \
        if(__a_unlikely(!flag)){                                                                            \
            memcpy((void*)f, bf, sizeof(__A_FUNC_BASE(T)));                                                 \
            if(A_SET_VTAB(T) != nullptr) A_SET_VTAB(T)(self);                                               \
            ((A_FUNC(Atlan)*)f)->dest = (void*)__A_OBJ_DEST_FUNC_SELF(T);                                   \
        }                                                                                                   \
                                                                                                            \
        if(!aExcOccur() && A_OBJ_INIT(T) != nullptr) A_OBJ_INIT(T)(self);                                   \
        if(aExcOccur()) __A_OBJ_DEST_FUNC_SELF(T)(self);                                                    \
    }                                                                                                       \
    __noused static inline void __A_OBJ_COPY_FUNC_SELF(T)(T* self, const T* that){                          \
        aExcClean();                                                                                        \
        if(__a_unlikely(self == nullptr)) { aExcSet(AEXC_nullptr); return; }                                \
                                                                                                            \
        if(that == nullptr){ __A_OBJ_INIT_FUNC_SELF(T)(self); return; }                                     \
                                                                                                            \
        A_FUNC(T)* f = (void*)&A_FUNC_TAB(T); __A_FUNC_BASE(T)* bf = (void*)__A_FUNC_TAB_BASE(T);           \
        memset(self, 0, sizeof(T));  auto flag = f->flag; self->f = f;                                      \
        __A_OBJ_COPY_FUNC_BASE(T)((void*)self, (const void*)that); self->f = f;                             \
                                                                                                            \
        if(__a_unlikely(!flag)){                                                                            \
            memcpy((void*)f, bf, sizeof(__A_FUNC_BASE(T)));                                                 \
            if(A_SET_VTAB(T) != nullptr) A_SET_VTAB(T)(self);                                               \
            ((A_FUNC(Atlan)*)f)->dest = (void*)__A_OBJ_DEST_FUNC_SELF(T);                                   \
        }                                                                                                   \
                                                                                                            \
        if(aExcOccur()){ __A_OBJ_DEST_FUNC_SELF(T)(self); return; }                                         \
                                                                                                            \
        if(A_OBJ_COPY(T) != nullptr){                                                                       \
            A_OBJ_COPY(T)(self, that);                                                                      \
        }else{                                                                                              \
            uint32_t size_self = sizeof(T); uint32_t size_base = sizeof(__A_CLASS_BASE(T));                 \
            memcpy(((char*)self) + size_base, ((const char*)that) + size_base, size_self - size_base);      \
        }                                                                                                   \
                                                                                                            \
        if(aExcOccur()) __A_OBJ_DEST_FUNC_SELF(T)(self);                                                    \
    }                                                                                                       \
    __noused static inline int  __A_OBJ_CMPD_FUNC_SELF(T)(const T* self, const T* that){                    \
        if(self == that || (self == nullptr && that == nullptr)){ return 0; }                               \
        if(self == nullptr){ return 1; } if(that == nullptr){ return -1; }                                  \
        int ret = __A_OBJ_CMPD_FUNC_BASE(T)((const void*)self, (const void*)that);if(ret != 0){return ret; }\
        return A_OBJ_CMPD(T) != nullptr ? A_OBJ_CMPD(T)(self, that):__A_OBJ_CMPD_AUTO(T, self, that);       \
    }                                                                                                       \



#define __AClass_Inherit(T, B, ...)                                                                         \
    static_assert(__A_IS_CLASS(B) != 0, "Error: The inherited type is not a class");                        \
    typedef B __A_CLASS_BASE(T);                                                                            \
    typedef A_FUNC(B) __A_FUNC_BASE(T);                                                                     \
    static auto __A_FUNC_TAB_BASE(T) = &A_FUNC_TAB(B);                                                      \
    static auto __A_OBJ_INIT_FUNC_BASE(T) = __A_OBJ_INIT_FUNC_SELF(B);                                      \
    static auto __A_OBJ_DEST_FUNC_BASE(T) = __A_OBJ_DEST_FUNC_SELF(B);                                      \
    static auto __A_OBJ_COPY_FUNC_BASE(T) = __A_OBJ_COPY_FUNC_SELF(B);                                      \
    static auto __A_OBJ_CMPD_FUNC_BASE(T) = __A_OBJ_CMPD_FUNC_SELF(B);                                      \



#define __A_CLASS_BASE(T)           __A_Splice(__A_CLASS_BASE_$__, T)
#define __A_FUNC_BASE(T)            __A_Splice(__A_FUNC_BASE_$__, T)
#define __A_FUNC_TAB_BASE(T)        __A_Splice(__A_FUNC_TAB_BASE_$__, T)

#define __A_OBJ_INIT_FUNC_BASE(T)   __A_Splice(__A_OBJ_INIT_FUNC_BASE_$__, T)
#define __A_OBJ_DEST_FUNC_BASE(T)   __A_Splice(__A_OBJ_DEST_FUNC_BASE_$__, T)
#define __A_OBJ_COPY_FUNC_BASE(T)   __A_Splice(__A_OBJ_COPY_FUNC_BASE_$__, T)
#define __A_OBJ_CMPD_FUNC_BASE(T)   __A_Splice(__A_OBJ_CMPD_FUNC_BASE_$__, T)

#define __A_CALL(obj, T, ...) (*(typeof(((typeof(T)*)0)->f))((obj).f))

#define A_CALL(obj, ...) ({                                                                                 \
    static_assert(__aNarg_n(0, ##__VA_ARGS__) <= 1,"Too many parameters");                                  \
    auto __a_obj = (obj); __A_CALL(__a_obj, ##__VA_ARGS__, __a_obj);                                        \
})                                                                                                          \

#define A_COVER_FUNC(self, T, name, func) ({                                                                \
    auto __a_self = (T*)(self); auto __a_f = (func);                                                        \
    A_FUNC(T)*__a_tab = (void*)(__a_self->f); __a_tab->name = (void*)__a_f;                                 \
})                                                                                                          \



/*
 * void A_SET_VTAB(T)(T* self){
 *      A_COVER_FUNC(self, B, name, func);
 * }
 */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __aclass_h__ */

