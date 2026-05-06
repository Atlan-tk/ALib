/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __alib_type_h__
#define __alib_type_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __cplusplus

#if !(defined(__STDC_VERSION__)) || (__STDC_VERSION__ < 201112L)
    #error "The minimum supported C standard is C11"
#endif /* __STDC_VERSION__ > c11 */

#if !defined(__GNUC__)
    #error "Please use a compiler that supports GNU extensions: gcc, clang, icc, armcc"
#endif /* gcc or clang */

#endif /* no __cplusplus */


#include <string.h>
#include <stdint.h>
#include <stdatomic.h>

/* utils */
/********************************************************************/
#define __aPagSize ((uint32_t)512)
#define __aCahSize ((uint32_t)64)


#ifndef __weak
#define __weak __attribute__((weak))
#endif /* __weak */

#ifndef __weakref
#define __weakref(symbol) __attribute__((weakref(__A_Str(symbol))))
#endif /* __weakref */

#ifndef __alias
#define __alias(symbol) __attribute__((alias(__A_Str(symbol))))
#endif /* __alias */

#ifndef __unused
#define __unused __attribute__((unused))
#endif /* __unused */

#ifndef __cleanup
#define __cleanup(func) __attribute__((cleanup(func)))
#endif /* __cleanup */

#ifndef __aligned
#define __aligned(n) __attribute__((aligned(n)))
#endif /* __aligned */

#ifndef typeof
#define typeof __typeof__
#endif /* typeof */

#ifndef container_of
#define container_of(ptr, type, member) ({                          \
    const typeof(((type *)0)->member) *__mptr = (ptr);              \
    (type *)((char *)__mptr - offsetof(type, member));              \
})
#endif /* container_of */

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif /* offsetof */

#define __a_type_check(T, obj)                                      \
    _Generic(*(typeof(obj)*)0, typeof(T): true, default: false)     \

#define __a_type_assert(T, obj)                                     \
    static_assert(__a_type_check(T, (obj)), "Error: Type mismatch");\

#define __a_argn_assert(n, ...)                                     \
    static_assert( __aNarg_n(0, ##__VA_ARGS__) <= n,                \
            "Error: Too many parameters");                          \

#define __a_likely(x)   __builtin_expect(!!(x), true)
#define __a_unlikely(x) __builtin_expect(!!(x), false)

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ < 202311L)

#include <stdbool.h>

#ifndef auto
#define auto __auto_type
#endif /* auto */

#ifndef alignas
#define alignas _Alignas
#endif /* alignas */

#ifndef alignof
#define alignof _Alignof
#endif /* alignof */

#ifndef nullptr
#define nullptr NULL
#endif /* nullptr */

#ifndef thread_local
#define thread_local _Thread_local
#endif /* thread_local */

#ifndef static_assert
#define static_assert _Static_assert
#endif /* static_assert */

#endif /* __STDC_VERSION__ < c23 */

__unused void  alib_free(void* p);
__unused void* alib_alloc(uint32_t size);
__unused void* alib_realloc(void* p, uint32_t size);

__unused void* alib_new(uint32_t size, void(*init_func)(void*));
__unused void* alib_new_for_copy(uint32_t size, const void* that, void(*copy_func)(void*, const void*));
__unused void  alib_delete(void* p, void(*dest_func)(void*));

__unused void* alib_ref_new(uint32_t size, void(*init_func)(void*));
__unused void* alib_ref_new_for_copy(uint32_t size, const void* that, void(*copy_func)(void*, const void*));
__unused void  alib_ref_delete(void* p, void(*dest_func)(void*));
__unused void* alib_ref_copy(void* p);

typedef void*               cptr_t;
typedef char*               cstr_t;
typedef long long           longlong;

typedef struct{
    const char* s;  //以0结尾
    uint32_t len;   //不包含0的长度
}astr_t;

static inline astr_t astr_new(const char* s){
    return (astr_t){ .s = s, .len = strlen(s) };
}



/* exception handling */
/********************************************************************/

enum AEXC_t{
    AEXC_NORMAL = 0,

    AEXC_nullptr = -1,
    AEXC_overstep = -2,
    AEXC_outdomain = -3,
    AEXC_init_failed = -4,
    AEXC_file_noexist = -5,
    AEXC_alloc_failed = -6,
    AEXC_no_permissions = -7,
    AEXC_invalid_function = -8,
    AEXC_repeat_write = -9,
};
extern thread_local int __A_EXC_VALUE__;
static inline void aExcClean() { __A_EXC_VALUE__ = 0; }
static inline bool aExcOccur() { return __A_EXC_VALUE__ != 0; };
static inline void aExcSet(enum AEXC_t v) { __A_EXC_VALUE__ = v; };
static inline int  aExcGet() { return __A_EXC_VALUE__; };



/* auxiliary macro */
/********************************************************************/
#define __A_3Splice(X, TK, TV) __A_Splice(X, __A_Splice(__A_Splice(TK, _$_), TV))
#define __A_Splice(arg0, arg1) ____A_Splice(arg0, arg1)
#define ____A_Splice(arg0, arg1) arg0##arg1

#define __A_Str(arg0) ____A_Str(arg0)
#define ____A_Str(arg0) #arg0

#define __aNarg_n(_0, ...) __aRuler(0, ##__VA_ARGS__,           \
    69, 68, 67, 66, 65, 64, 63, 62, 61, 60,                     \
    59, 58, 57, 56, 55, 54, 53, 52, 51, 50,                     \
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40,                     \
    39, 38, 37, 36, 35, 34, 33, 32, 31, 30,                     \
    29, 28, 27, 26, 25, 24, 23, 22, 21, 20,                     \
    19, 18, 17, 16, 15, 14, 13, 12, 11, 10,                     \
     9,  8,  7,  6,  5,  4,  3,  2,  1,  0                      \
)                                                               \

#define __aRuler(                                               \
     _0,  _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9,           \
    _10, _11, _12, _13, _14, _15, _16, _17, _18, _19,           \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29,           \
    _30, _31, _32, _33, _34, _35, _36, _37, _38, _39,           \
    _40, _41, _42, _43, _44, _45, _46, _47, _48, _49,           \
    _50, _51, _52, _53, _54, _55, _56, _57, _58, _59,           \
    _60, _61, _62, _63, _64, _65, _66, _67, _68, _69,           \
    __n, ...                                                    \
) (__n)                                                         \



/* type system */
/********************************************************************/
#define A_TYPE_REGISTER(T)                                                                                  \
    void A_OBJ_DEST(T)(T* self);                                                                            \
    void A_OBJ_INIT(T)(T* self);                                                                            \
    void A_OBJ_COPY(T)(T* self, const T* that);                                                             \
    int  A_OBJ_CMPD(T)(const T* self, const T* that);                                                       \
    uint32_t  A_OBJ_HASH(T)(const T* self);                                                                 \
                                                                                                            \
    static void __A_OBJ_DEST(T)(T* self) __weakref(A_OBJ_DEST(T));                                          \
    static void __A_OBJ_INIT(T)(T* self) __weakref(A_OBJ_INIT(T));                                          \
    static void __A_OBJ_COPY(T)(T* self, const T* that) __weakref(A_OBJ_COPY(T));                           \
    static int  __A_OBJ_CMPD(T)(const T* self, const T* that) __weakref(A_OBJ_CMPD(T));                     \
    __unused static uint32_t __A_OBJ_HASH(T)(const T* self) __weakref(A_OBJ_HASH(T));                       \
                                                                                                            \
    __unused static inline void __A_OBJ_DEST_FUNC_SELF(T)(T* self){                                         \
        /* 空对象不析构 */                                                                                  \
        if(__a_unlikely(self == nullptr)) { return; }                                                       \
                                                                                                            \
        T null_obj; memset(&null_obj, 0, sizeof(T));                                                        \
        if(memcmp(self, &null_obj, sizeof(T)) == 0){ return; }                                              \
                                                                                                            \
        if(__A_OBJ_DEST(T) != nullptr) __A_OBJ_DEST(T)(self);                                               \
    }                                                                                                       \
    __unused static inline void __A_OBJ_INIT_FUNC_SELF(T)(T* self){                                         \
        aExcClean();                                                                                        \
        if(__a_unlikely(self == nullptr)) { aExcSet(AEXC_nullptr); return; }                                \
        memset(self, 0, sizeof(T)); if(__A_OBJ_INIT(T) != nullptr) __A_OBJ_INIT(T)(self);                   \
        if(aExcOccur()) __A_OBJ_DEST_FUNC_SELF(T)(self);                                                    \
    }                                                                                                       \
    __unused static inline void __A_OBJ_COPY_FUNC_SELF(T)(T* self, const T* that){                          \
        aExcClean();                                                                                        \
        if(__a_unlikely(self == nullptr)) { aExcSet(AEXC_nullptr); return; }                                \
        if(__a_unlikely(that == nullptr)) { __A_OBJ_INIT_FUNC_SELF(T)(self); return; }                      \
        memset(self, 0, sizeof(T)); if(__A_OBJ_COPY(T) != nullptr) __A_OBJ_COPY(T)(self, that);             \
        if(aExcOccur()) __A_OBJ_DEST_FUNC_SELF(T)(self);                                                    \
    }                                                                                                       \
    __unused static inline int __A_OBJ_CMPD_FUNC_SELF(T)(const T* self, const T* that){                     \
        if(self == that || (self == nullptr && that == nullptr)) return 0;                                  \
        if(self == nullptr){ return 1; } if(that == nullptr){ return -1; }                                  \
        return __A_OBJ_CMPD(T) != nullptr ? __A_OBJ_CMPD(T)(self,that) : __A_OBJ_CMPD_AUTO(T,self,that);    \
    }                                                                                                       \



#define __A_OBJ_INIT_FUNC_SELF(T) __A_Splice(__A_OBJ_INIT_FUNC_SELF_$__, T)
#define __A_OBJ_COPY_FUNC_SELF(T) __A_Splice(__A_OBJ_COPY_FUNC_SELF_$__, T)
#define __A_OBJ_DEST_FUNC_SELF(T) __A_Splice(__A_OBJ_DEST_FUNC_SELF_$__, T)
#define __A_OBJ_CMPD_FUNC_SELF(T) __A_Splice(__A_OBJ_CMPD_FUNC_SELF_$__, T)

#define A_OBJ_INIT(T) __A_Splice(T, _init)
#define A_OBJ_DEST(T) __A_Splice(T, _dest)
#define A_OBJ_COPY(T) __A_Splice(T, _copy)
#define A_OBJ_CMPD(T) __A_Splice(T, _cmpd)
#define A_OBJ_HASH(T) __A_Splice(T, _hash)

#define __A_OBJ_INIT(T) __A_Splice(____A_OBJ_INIT_$__, T)
#define __A_OBJ_DEST(T) __A_Splice(____A_OBJ_DEST_$__, T)
#define __A_OBJ_COPY(T) __A_Splice(____A_OBJ_COPY_$__, T)
#define __A_OBJ_CMPD(T) __A_Splice(____A_OBJ_CMPD_$__, T)
#define __A_OBJ_HASH(T) __A_Splice(____A_OBJ_HASH_$__, T)

#define __A_OBJ_CMPD_X(T, self, that) ({                                    \
    const T* __a_p0 = (const void*)self;                                    \
    const T* __a_p1 = (const void*)that;                                    \
    int __a_ret = *__a_p0 == *__a_p1 ? 0 : (*__a_p0 > *__a_p1 ? 1 : -1);    \
    __a_ret;                                                                \
})                                                                          \

#define __A_OBJ_CMPD_AUTO(T, self, that)                                    \
    _Generic(*(typeof(T*))0,                                                \
        int8_t:  __A_OBJ_CMPD_X(int8_t,   (self), (that)),                  \
        int16_t: __A_OBJ_CMPD_X(int16_t,  (self), (that)),                  \
        int32_t: __A_OBJ_CMPD_X(int32_t,  (self), (that)),                  \
        int64_t: __A_OBJ_CMPD_X(int64_t,  (self), (that)),                  \
        uint8_t: __A_OBJ_CMPD_X(uint8_t,  (self), (that)),                  \
        uint16_t:__A_OBJ_CMPD_X(uint16_t, (self), (that)),                  \
        uint32_t:__A_OBJ_CMPD_X(uint32_t, (self), (that)),                  \
        uint64_t:__A_OBJ_CMPD_X(uint64_t, (self), (that)),                  \
        bool:    __A_OBJ_CMPD_X(bool,     (self), (that)),                  \
        float:   __A_OBJ_CMPD_X(float,    (self), (that)),                  \
        double:  __A_OBJ_CMPD_X(double,   (self), (that)),                  \
        cptr_t:  __A_OBJ_CMPD_X(size_t,   (self), (that)),                  \
        cstr_t:  strcmp(*(cstr_t*)(self), *(cstr_t*)(that)),                \
        astr_t:  strcmp(*(cstr_t*)(self), *(cstr_t*)(that)),                \
        default: memcmp((self), (that), sizeof(T))                          \
)                                                                           \

#define A_INIT(T)({                                                         \
    T __a_obj; __A_OBJ_INIT_FUNC_SELF(T)(&__a_obj); __a_obj;                \
})                                                                          \

#define A_DEST(T, obj)({                                                    \
    auto __a_obj = (obj);                                                   \
    __a_type_assert(T, __a_obj);                                            \
    T __a_objx = __a_obj; __A_OBJ_DEST_FUNC_SELF(T)(&__a_objx);             \
})                                                                          \

#define A_CMPD(T, obj0, obj1)({                                             \
    auto __a_obj0 = (obj0); auto __a_obj1 = (obj1);                         \
    __a_type_assert(T, __a_obj0); __a_type_assert(T, __a_obj1);             \
    __A_OBJ_CMPD_FUNC_SELF(T)(&__a_obj0, &__a_obj1);                        \
})                                                                          \

#define A_COPY(T, obj)({                                                    \
    auto __a_obj = (obj);                                                   \
    __a_type_assert(T, __a_obj);                                            \
    T __a_objx; __A_OBJ_COPY_FUNC_SELF(T)(&__a_objx, &__a_obj); __a_objx;   \
})                                                                          \

#define A_MOVE(obj)({                                                       \
    /* 参数 obj 只能为非const的右值 */                                      \
    auto __a_p = &(obj);                                                    \
    typeof(*__a_p)__a_obj = *__a_p; memset(__a_p,0,sizeof(*__a_p));__a_obj; \
})                                                                          \



/* raii */
#define RAII(T) __cleanup(__A_OBJ_DEST_FUNC_SELF(T)) T

#define A_NEW(T) ({                                                         \
    (T*)alib_new(sizeof(T), (void*)__A_OBJ_INIT_FUNC_SELF(T));              \
})                                                                          \

#define A_NEW_COPY(T, obj) ({                                               \
    auto __a_objx = (obj);                                                  \
    __a_type_assert(T, __a_objx);                                           \
    (T*)alib_new_for_copy(sizeof(T),  &__a_objx,                            \
            (void*)__A_OBJ_COPY_FUNC_SELF(T));                              \
})                                                                          \

#define A_DELETE(T, _p) ({                                                  \
    auto __a_p = (_p); __a_type_assert(T, *__a_p);                          \
    alib_delete((_p), (void*)__A_OBJ_DEST_FUNC_SELF(T));                    \
})                                                                          \

//RAII(int) obj = A_INIT(int);
//return;//自动释放

//int* obj = A_NEW(int);
//A_DELETE(int, obj);//堆上对象手动释放
//return;



/* PRIMITIVE TYPE REGISTER */
/********************************************************************/
//void
__unused static inline void __A_OBJ_DEST_FUNC_SELF(void)(__unused void* self){}
__unused static inline void __A_OBJ_INIT_FUNC_SELF(void)(__unused void* self){}
__unused static inline void __A_OBJ_COPY_FUNC_SELF(void)(__unused void* self, __unused const void* that){}
__unused static inline int  __A_OBJ_CMPD_FUNC_SELF(void)(__unused const void* self, __unused const void* that){ return 0; }
__unused static inline uint32_t __A_OBJ_HASH(void)(__unused const void* self){ return 0; };

//int
#define __A_PRIMITIVE_TYPE_REGISTER(T)                                                  \
    __unused static void __A_OBJ_DEST_FUNC_SELF(T)(__unused T* self){                   \
    }                                                                                   \
    __unused static void  __A_OBJ_INIT_FUNC_SELF(T)(T* self){                           \
        if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }             \
        memset(self, 0, sizeof(T));                                                     \
    }                                                                                   \
    __unused static void  __A_OBJ_COPY_FUNC_SELF(T)(T* self, const T* that){            \
        if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }             \
        memset(self, 0, sizeof(T)); if(that != nullptr) *self = *that;                  \
    }                                                                                   \
    __unused static int __A_OBJ_CMPD_FUNC_SELF(T)(const T* self, const T* that){        \
        if(self == that || (self == nullptr && that == nullptr)) return 0;              \
        if(self == nullptr){ return 1; } if(that == nullptr){ return -1; }              \
        return __A_OBJ_CMPD_AUTO(T,self,that);                                          \
    }                                                                                   \
    __unused static uint32_t (*const __A_OBJ_HASH(T))(const T* self) = nullptr;         \

__A_PRIMITIVE_TYPE_REGISTER(int);
__A_PRIMITIVE_TYPE_REGISTER(bool);
__A_PRIMITIVE_TYPE_REGISTER(char);
__A_PRIMITIVE_TYPE_REGISTER(long);
__A_PRIMITIVE_TYPE_REGISTER(short);
__A_PRIMITIVE_TYPE_REGISTER(float);
__A_PRIMITIVE_TYPE_REGISTER(double);
__A_PRIMITIVE_TYPE_REGISTER(size_t);

__A_PRIMITIVE_TYPE_REGISTER(int8_t);
__A_PRIMITIVE_TYPE_REGISTER(int16_t);
__A_PRIMITIVE_TYPE_REGISTER(int32_t);
__A_PRIMITIVE_TYPE_REGISTER(int64_t);
__A_PRIMITIVE_TYPE_REGISTER(uint8_t);
__A_PRIMITIVE_TYPE_REGISTER(uint16_t);
__A_PRIMITIVE_TYPE_REGISTER(uint32_t);
__A_PRIMITIVE_TYPE_REGISTER(uint64_t);

__A_PRIMITIVE_TYPE_REGISTER(cptr_t);
__A_PRIMITIVE_TYPE_REGISTER(cstr_t);
__A_PRIMITIVE_TYPE_REGISTER(astr_t);

uint32_t alib_hash(const void* k, uint32_t size_k);
uint32_t alib_hash_str(const char* k);



/* function table */
/********************************************************************/
#define A_FUNC(T)       __A_Splice(__A_FUNCTION_$__, T)
#define A_FUNC_TAB(T)   __A_Splice(__A_FUNCTION_$_TAB__, T)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __alib_type_h__ */

