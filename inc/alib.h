/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __alib_type_h__
#define __alib_type_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(__C_WINDOWS__) && !defined(__C_POSIX__)
    #if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        #define __C_WINDOWS__ 1
    #elif defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__)
        #define __C_POSIX__ 1
    #else
        #error "Unsupported platform: define __C_POSIX__ or __C_WINDOWS__ before including alib.h"
    #endif
#endif

#ifndef __cplusplus
    #if !(defined(__STDC_VERSION__)) || (__STDC_VERSION__ < 201112L)
        #error "The minimum supported C standard is C11"
    #endif /* __STDC_VERSION__ > c11 */

    #if !defined(__GNUC__) && !defined(__clang__)
        #error "Please use a compiler that supports GNU extensions: gcc, clang, icc, armcc"
    #endif /* gcc or clang */

    #if defined(__STDC_VERSION__) && (__STDC_VERSION__ < 202000L)
        #include <stddef.h>
        #include <stdbool.h>

        #ifndef auto
            #define auto __auto_type
        #endif /* auto */

        #ifndef nullptr
            #define nullptr NULL
        #endif /* nullptr */

        #ifndef thread_local
            #define thread_local _Thread_local
        #endif /* thread_local */

        #ifndef static_assert
            #define static_assert _Static_assert
        #endif /* static_assert */

        #ifndef typeof
            #define typeof __typeof__
        #endif /* typeof */

        #ifndef noreturn
            #define noreturn _Noreturn
            #define __noreturn noreturn
        #endif
    #endif /* __STDC_VERSION__ < c23 */
#endif /* no __cplusplus */

#ifndef __weak
    #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
        #define __weak [[gnu::weak]]
    #else
        #define __weak __attribute__((weak))
    #endif
#endif /* __weak */

#ifndef __weakref
    #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
        #define __weakref(symbol) [[gnu::weakref(__A_Str(symbol))]]
    #else
        #define __weakref(symbol) __attribute__((weakref(__A_Str(symbol))))
    #endif
#endif /* __weakref */

#ifndef __noused
    #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
        #define __noused [[gnu::unused]]
    #else
        #define __noused __attribute__((unused))
    #endif
#endif /* __noused */

#ifndef __cleanup
    #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
        #define __cleanup(func) [[gnu::cleanup(func)]]
    #else
        #define __cleanup(func) __attribute__((cleanup(func)))
    #endif
#endif /* __cleanup */

#ifndef __noreturn
    #if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)) || defined(__cplusplus)
        #define __noreturn [[noreturn]]
    #else
        #define __noreturn __attribute__((noreturn))
    #endif
#endif /* __noreturn */

#ifndef __visibility
    #if defined(__C_WINDOWS__)
        #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
            #define __visibility(mode) [[gnu::visibility(__A_Str(default))]]
        #else
            #define __visibility(mode) __attribute__((visibility(__A_Str(default))))
        #endif
    #endif /* windows */

    #if defined(__C_POSIX__)
        #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)
            #define __visibility(mode) [[gnu::visibility(__A_Str(mode))]]
        #else
            #define __visibility(mode) __attribute__((visibility(__A_Str(mode))))
        #endif
    #endif /* posix */
#endif /* __visibility */

#include <string.h>
#include <stdint.h>



/* utils */
/********************************************************************/
#define __aPagSize ((uint32_t)512)
#define __aCahSize ((uint32_t)64 )

#ifndef container_of
    #define container_of(ptr, type, member) ({                      \
        const typeof(((type *)0)->member) *__mptr = (ptr);          \
        (type *)((char *)__mptr - offsetof(type, member));          \
    })
#endif /* container_of */

#ifndef offsetof
    #define offsetof(type, member) __builtin_offsetof(type, member)
#endif /* offsetof */

#define __a_type_check(T, obj)                                      \
    _Generic(*(typeof(obj)*)0, typeof(T): true, default: false)     \

#define __a_type_assert(T, obj)({                                   \
    static_assert(__a_type_check(T, (obj)), "Error: Type mismatch");\
})                                                                  \

#define __a_argn_assert(n, ...)                                     \
    static_assert( __aNarg_n(0, ##__VA_ARGS__) <= n,                \
            "Error: Too many parameters");                          \

#define __a_likely(x)   __builtin_expect(!!(x), true)
#define __a_unlikely(x) __builtin_expect(!!(x), false)



/* alloc and free */
#if defined(__C_WINDOWS__)
extern void  (*const alib_free)(void* p);
extern void* (*const alib_alloc)(uint32_t size);
extern void* (*const alib_realloc)(void* p, uint32_t size);
extern void* (*const alib_new)(uint32_t size, void(*init_func)(void*));
extern void* (*const alib_cpnew)(uint32_t size, const void* that, void(*copy_func)(void*, const void*));
extern void  (*const alib_delete)(void* p, void(*dest_func)(void*));
#endif /* windows */

#if defined(__C_POSIX__)
void  alib_free(void* p);
void* alib_alloc(uint32_t size);
void* alib_realloc(void* p, uint32_t size);
void* alib_new(uint32_t size, void(*init_func)(void*));
void* alib_cpnew(uint32_t size, const void* that, void(*copy_func)(void*, const void*));
void  alib_delete(void* p, void(*dest_func)(void*));
#endif /* posix */



typedef void*               cptr_t;
typedef char*               cstr_t;
typedef long long           longlong;

typedef struct{
    const char* s;  //以0结尾
    uint32_t len;   //不包含0的长度
}astr_t;

static inline astr_t astr_new(const char* s){
    return (astr_t){ .s = s, .len = (uint32_t)strlen(s) };
}

static inline int __a_strcmp(const char* s0, const char* s1){
    if(s0 == s1) return 0;
    if(s0 == nullptr) return -1;
    if(s1 == nullptr) return 1;
    return strcmp(s0, s1);
}

static inline int __a_memcmp(const void* s0, const void* s1, uint32_t size){
    if(s0 == s1) return 0;
    if(s0 == nullptr) return -1;
    if(s1 == nullptr) return 1;
    return memcmp(s0, s1, size);
}

__noused static inline void a_class_dest(void* _self);
uint32_t alib_hash(const void* k, uint32_t size_k);
uint32_t alib_hash_str(const char* k);




/* exception handling */
/********************************************************************/
thread_local extern int __a_err_value__;
typedef enum{
    AERR_NORMAL = 0,            //正常

    AERR_nullptr = 65536,       //self指针为空
    AERR_overstep,              //越界访问
    AERR_outdomain,             //函数参数超出预定范围
    AERR_init_failed,           //初始化失败
    AERR_file_noexist,          //文件不存在
    AERR_alloc_failed,          //内存分配失败
    AERR_no_permissions,        //无操作权限，常用于文件系统或设备
    AERR_invalid_function,      //已废弃的函数
    AERR_repeat_write,          //重复写入
    AERR_system_error,          //系统错误
    AERR_response_exc,          //信号响应异常
    AERR_matching_failed,       //常用于信号匹配失败
    AERR_timedout,              //超时
    AERR_thrd_err,              //线程错误返回
}AERR_t;
static inline void aErrClean(void)  { __a_err_value__ = 0; }
static inline bool aErrOccur(void)  { return __a_err_value__ != 0; };
static inline void aErrSet(AERR_t v){ __a_err_value__ = v; };
static inline int  aErrGet(void)    { return __a_err_value__; };

#define aTry(...)   aErrClean(); __VA_ARGS__; if(!aErrOccur()){}
#define aHit(ev)    else if(aErrGet() == (ev))
#define aExc        else



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
    enum{ __A_IS_CLASS(T) = 0 };                                                                            \
                                                                                                            \
    void A_OBJ_DEST(T)(T* self);                                                                            \
    void A_OBJ_INIT(T)(T* self);                                                                            \
    void A_OBJ_COPY(T)(T* self, const T* that);                                                             \
    int  A_OBJ_CMPD(T)(const T* self, const T* that);                                                       \
                                                                                                            \
    __weakref(A_OBJ_DEST(T)) static void __A_OBJ_DEST(T)(T* self);                                          \
    __weakref(A_OBJ_INIT(T)) static void __A_OBJ_INIT(T)(T* self);                                          \
    __weakref(A_OBJ_COPY(T)) static void __A_OBJ_COPY(T)(T* self, const T* that);                           \
    __weakref(A_OBJ_CMPD(T)) static int  __A_OBJ_CMPD(T)(const T* self, const T* that);                     \
                                                                                                            \
    __noused static inline void __A_OBJ_DEST_FUNC_SELF(T)(T* self){                                         \
        /* 空对象不析构 */                                                                                  \
        if(__a_unlikely(self == nullptr)) { return; }                                                       \
                                                                                                            \
        T null_obj; memset(&null_obj, 0, sizeof(T));                                                        \
        if(memcmp(self, &null_obj, sizeof(T)) == 0){ return; }                                              \
                                                                                                            \
        if(__A_OBJ_DEST(T) != nullptr) __A_OBJ_DEST(T)(self);                                               \
        memset(self, 0, sizeof(T));                                                                         \
    }                                                                                                       \
    __noused static inline void __A_OBJ_INIT_FUNC_SELF(T)(T* self){                                         \
        aErrClean();                                                                                        \
        if(__a_unlikely(self == nullptr)) { aErrSet(AERR_nullptr); return; }                                \
        memset(self, 0, sizeof(T)); if(__A_OBJ_INIT(T) != nullptr) __A_OBJ_INIT(T)(self);                   \
        if(aErrOccur()) __A_OBJ_DEST_FUNC_SELF(T)(self);                                                    \
    }                                                                                                       \
    __noused static inline void __A_OBJ_COPY_FUNC_SELF(T)(T* self, const T* that){                          \
        aErrClean();                                                                                        \
        if(__a_unlikely(self == nullptr)) { aErrSet(AERR_nullptr); return; }                                \
        if(__a_unlikely(that == nullptr)) { __A_OBJ_INIT_FUNC_SELF(T)(self); return; }                      \
        memset(self, 0, sizeof(T)); if(__A_OBJ_COPY(T) != nullptr){                                         \
            __A_OBJ_COPY(T)(self, that);                                                                    \
        }else{                                                                                              \
            memcpy(self, that, sizeof(T));                                                                  \
        }                                                                                                   \
        if(aErrOccur()) __A_OBJ_DEST_FUNC_SELF(T)(self);                                                    \
    }                                                                                                       \
    __noused static inline int __A_OBJ_CMPD_FUNC_SELF(T)(const T* self, const T* that){                     \
        if(self == that || (self == nullptr && that == nullptr)) return 0;                                  \
        if(self == nullptr){ return 1; } if(that == nullptr){ return -1; }                                  \
        return __A_OBJ_CMPD(T) != nullptr ? __A_OBJ_CMPD(T)(self,that) : __A_OBJ_CMPD_AUTO(T,self,that);    \
    }                                                                                                       \



#define __A_OBJ_INIT_FUNC_SELF(T) __A_Splice(__A_OBJ_INIT_FUNC_SELF_$__, T)
#define __A_OBJ_COPY_FUNC_SELF(T) __A_Splice(__A_OBJ_COPY_FUNC_SELF_$__, T)
#define __A_OBJ_DEST_FUNC_SELF(T) __A_Splice(__A_OBJ_DEST_FUNC_SELF_$__, T)
#define __A_OBJ_CMPD_FUNC_SELF(T) __A_Splice(__A_OBJ_CMPD_FUNC_SELF_$__, T)
#define __A_IS_CLASS(T)           __A_Splice(__A_IS_CLASS_$__, T)

#define A_OBJ_INIT(T) __A_Splice(T, _init)
#define A_OBJ_DEST(T) __A_Splice(T, _dest)
#define A_OBJ_COPY(T) __A_Splice(T, _copy)
#define A_OBJ_CMPD(T) __A_Splice(T, _cmpd)
#define A_OBJ_HASH(T) __A_Splice(T, _hash)
#define A_SET_VTAB(T) __A_Splice(T, _vtab)

#define __A_OBJ_INIT(T) __A_Splice(T, _init_ref)
#define __A_OBJ_DEST(T) __A_Splice(T, _dest_ref)
#define __A_OBJ_COPY(T) __A_Splice(T, _copy_ref)
#define __A_OBJ_CMPD(T) __A_Splice(T, _cmpd_ref)
#define __A_OBJ_HASH(T) __A_Splice(T, _hash_ref)
#define __A_SET_VTAB(T) __A_Splice(T, _vtab_ref)

#define __A_OBJ_CMPD_X(T, self, that) ({                                            \
    const T* __a_p0 = (const void*)self;                                            \
    const T* __a_p1 = (const void*)that;                                            \
    int __a_ret = *__a_p0 == *__a_p1 ? 0 : (*__a_p0 > *__a_p1 ? 1 : -1);            \
    __a_ret;                                                                        \
})                                                                                  \

#define __A_OBJ_CMPD_AUTO(T, self, that)                                            \
    _Generic(*(typeof(T*))0,                                                        \
        int8_t:  __A_OBJ_CMPD_X(int8_t,   (self), (that)),                          \
        int16_t: __A_OBJ_CMPD_X(int16_t,  (self), (that)),                          \
        int32_t: __A_OBJ_CMPD_X(int32_t,  (self), (that)),                          \
        int64_t: __A_OBJ_CMPD_X(int64_t,  (self), (that)),                          \
        uint8_t: __A_OBJ_CMPD_X(uint8_t,  (self), (that)),                          \
        uint16_t:__A_OBJ_CMPD_X(uint16_t, (self), (that)),                          \
        uint32_t:__A_OBJ_CMPD_X(uint32_t, (self), (that)),                          \
        uint64_t:__A_OBJ_CMPD_X(uint64_t, (self), (that)),                          \
        bool:    __A_OBJ_CMPD_X(bool,     (self), (that)),                          \
        float:   __A_OBJ_CMPD_X(float,    (self), (that)),                          \
        double:  __A_OBJ_CMPD_X(double,   (self), (that)),                          \
        cptr_t:  __A_OBJ_CMPD_X(size_t,   (self), (that)),                          \
        cstr_t:  __a_strcmp(*(cstr_t*)(self), *(cstr_t*)(that)),                    \
        astr_t:  __a_strcmp(((astr_t*)(self))->s, ((astr_t*)(that))->s),            \
        default: __a_memcmp((self), (that), sizeof(T))                              \
)                                                                                   \

#define A_INIT(T)({                                                                 \
    T __a_obj; __A_OBJ_INIT_FUNC_SELF(T)(&__a_obj); __a_obj;                        \
})                                                                                  \

#define A_DEST(T, obj)({                                                            \
    /* 参数 obj 只能为非const的左值 */                                              \
    auto __a_p = &(obj); __a_type_assert(T, *__a_p);                                \
    (__A_IS_CLASS(T) != 0) ? a_class_dest(__a_p) : __A_OBJ_DEST_FUNC_SELF(T)(__a_p);\
})                                                                                  \

#define A_CMPD(T, obj0, obj1)({                                                     \
    auto __a_obj0 = (obj0); __a_type_assert(T, __a_obj0);                           \
    auto __a_obj1 = (obj1); __a_type_assert(T, __a_obj1);                           \
    __A_OBJ_CMPD_FUNC_SELF(T)(&__a_obj0, &__a_obj1);                                \
})                                                                                  \

#define A_COPY(T, obj)({                                                            \
    auto __a_obj = (obj); __a_type_assert(T, __a_obj);                              \
    T __a_objx; memset(&__a_objx, 0, sizeof(T));                                    \
    __A_OBJ_COPY_FUNC_SELF(T)(&__a_objx, &__a_obj); __a_objx;                       \
})                                                                                  \

#define A_MOVE(obj)({                                                               \
    /* 参数 obj 只能为非const的左值 */                                              \
    auto __a_p = &(obj);                                                            \
    typeof(*__a_p)__a_obj = *__a_p; memset(__a_p,0,sizeof(*__a_p));__a_obj;         \
})                                                                                  \

/* 右值转为左值 */
#define A_LEFT(obj) ((typeof(obj)[1]){ (obj) }[0])

/* raii */
#define RAII(T) __cleanup(__A_OBJ_DEST_FUNC_SELF(T)) T

#define A_NEW(T) ({                                                                 \
    (T*)alib_new(sizeof(T), (void*)__A_OBJ_INIT_FUNC_SELF(T));                      \
})                                                                                  \

#define A_CPNEW(T, obj) ({                                                          \
    auto __a_obj = (obj); __a_type_assert(T, __a_obj);                              \
    (T*)alib_cpnew(sizeof(T),  &__a_obj, (void*)__A_OBJ_COPY_FUNC_SELF(T));         \
})                                                                                  \

#define A_DELETE(T, _p) ({                                                          \
    auto __a_p = (_p); __a_type_assert(T, *__a_p);                                  \
    alib_delete((_p), (__A_IS_CLASS(T) != 0) ?                                      \
            (void*)a_class_dest : (void*)__A_OBJ_DEST_FUNC_SELF(T));                \
})                                                                                  \

//RAII(int) obj = A_INIT(int);
//return;//自动释放

//int* p = A_NEW(int);
//A_DELETE(int, p);//堆上对象手动释放
//return;



/* function table */
/********************************************************************/
#define A_FUNC(T)       __A_Splice(__A_FUNCTION_$__, T)
#define A_FUNC_TAB(T)   __A_Splice(__A_FUNCTION_$_TAB__, T)



/* PRIMITIVE TYPE REGISTER */
/********************************************************************/
//void
__noused static inline void __A_OBJ_DEST_FUNC_SELF(void)(__noused void* self){}
__noused static inline void __A_OBJ_INIT_FUNC_SELF(void)(__noused void* self){}
__noused static inline void __A_OBJ_COPY_FUNC_SELF(void)(__noused void* self, __noused const void* that){}
__noused static inline int  __A_OBJ_CMPD_FUNC_SELF(void)(__noused const void* self, __noused const void* that){ return 0; }

//int
#define __A_PRIMITIVE_TYPE_REGISTER(T)                                                                      \
    enum{ __A_IS_CLASS(T) = 0 };                                                                            \
    __noused static void __A_OBJ_DEST_FUNC_SELF(T)(__noused T* self){                                       \
        memset(self, 0, sizeof(T));                                                                         \
    }                                                                                                       \
    __noused static void  __A_OBJ_INIT_FUNC_SELF(T)(T* self){                                               \
        if(__a_unlikely(self == nullptr)){ aErrSet(AERR_nullptr); return; }                                 \
        memset(self, 0, sizeof(T));                                                                         \
    }                                                                                                       \
    __noused static void  __A_OBJ_COPY_FUNC_SELF(T)(T* self, const T* that){                                \
        if(__a_unlikely(self == nullptr)){ aErrSet(AERR_nullptr); return; }                                 \
        memset(self, 0, sizeof(T)); if(that != nullptr) *self = *that;                                      \
    }                                                                                                       \
    __noused static int __A_OBJ_CMPD_FUNC_SELF(T)(const T* self, const T* that){                            \
        if(self == that || (self == nullptr && that == nullptr)) return 0;                                  \
        if(self == nullptr){ return 1; } if(that == nullptr){ return -1; }                                  \
        return __A_OBJ_CMPD_AUTO(T,self,that);                                                              \
    }                                                                                                       \



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

//Atlan
typedef struct Atlan Atlan;
typedef struct A_FUNC(Atlan) A_FUNC(Atlan);

struct Atlan{
    const A_FUNC(Atlan)* f;
};

struct A_FUNC(Atlan){
    bool flag;
    void (*dest)(void*);
};

static const A_FUNC(Atlan) A_FUNC_TAB(Atlan);

enum{ __A_IS_CLASS(Atlan) = 1 };

__noused static inline void A_SET_VTAB(Atlan)(__noused Atlan* self){}

__noused static inline void __A_OBJ_DEST_FUNC_SELF(Atlan)(__noused Atlan* self){}

__noused static inline void __A_OBJ_INIT_FUNC_SELF(Atlan)(Atlan* self){
    if(__a_unlikely(self == nullptr)) { aErrSet(AERR_nullptr); return; }
    self->f = &A_FUNC_TAB(Atlan);
}

__noused static inline void __A_OBJ_COPY_FUNC_SELF(Atlan)(Atlan* self, const Atlan* that){
    if(__a_unlikely(self == nullptr)) { aErrSet(AERR_nullptr); return; }
    memset(self, 0, sizeof(Atlan)); if(that != nullptr) *self = *that;
}

__noused static inline int  __A_OBJ_CMPD_FUNC_SELF(Atlan)(const Atlan* self, const Atlan* that){
    if(self == that || (self == nullptr && that == nullptr)) return 0;
    if(self == nullptr){ return 1; } if(that == nullptr){ return -1; }
    return 0;
}

static const A_FUNC(Atlan) A_FUNC_TAB(Atlan) = { .flag = true, (void*)__A_OBJ_DEST_FUNC_SELF(Atlan) };

__noused static inline void a_class_dest(void* _self){
    Atlan* self = _self;

    if(self->f != nullptr && self->f->dest != nullptr)
        self->f->dest(self);
}


#define AEND ((uint32_t)0xffffffff)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __alib_type_h__ */
