/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <alib.h>
#include <stddef.h>
#include <stdlib.h>

/* memory management */
static void __alib_default_free(void* p){
    free(p);
}

static void* __alib_default_alloc(uint32_t size){
    return malloc(size);
}

static void* __alib_default_realloc(void* p, uint32_t size){
    return realloc(p, size);
}

static void* __alib_default_new(uint32_t size, void(*init_func)(void*)){
    aExcClean();
    void* p = alib_alloc(size);

    if(__a_unlikely(p == nullptr)){
        if(__a_likely(size != 0))
            aExcSet(AEXC_alloc_failed);
        return nullptr;
    }

    if(init_func != nullptr){
        init_func(p);
    }else{
        memset(p, 0, size);
    }

    if(aExcOccur()){
        alib_free(p); p = nullptr;
    }

    return p;
}

static void* __alib_default_cpnew(uint32_t size, const void* that, void(*copy_func)(void*, const void*)){
    aExcClean();
    void* p = alib_alloc(size);

    if(__a_unlikely(p == nullptr)){
        if(__a_likely(size != 0))
            aExcSet(AEXC_alloc_failed);
        return nullptr;
    }

    if(copy_func != nullptr){
        copy_func(p, that);
    }else{
        memset(p, 0, size);
    }

    if(aExcOccur()){
        alib_free(p); p = nullptr;
    }

    return p;
}

static void __alib_default_delete(void* p, void(*dest_func)(void*)){
    if(__a_likely(p != nullptr)){
        if(__a_likely(dest_func != nullptr)) dest_func(p);
        alib_free(p);
    }
}

#if defined (__C_WINDOWS__)
    #if defined (__ALIB_USER_ALLOC__)
        void  __alib_user_free(void* p);
        void* __alib_user_alloc(uint32_t size);
        void* __alib_user_realloc(void* p, uint32_t size);
        void* __alib_user_new(uint32_t size, void(*init_func)(void*));
        void* __alib_user_cpnew(uint32_t size, const void* that, void(*copy_func)(void*, const void*));
        void  __alib_user_delete(void* p, void(*dest_func)(void*));

        void  (*const alib_free)(void* p) = __alib_user_free;
        void* (*const alib_alloc)(uint32_t size) = __alib_user_alloc;
        void* (*const alib_realloc)(void* p, uint32_t size) = __alib_user_realloc;
        void* (*const alib_new)(uint32_t size, void(*init_func)(void*)) = __alib_user_new;
        void* (*const alib_cpnew)(uint32_t size, const void* that, void(*copy_func)(void*, const void*)) = __alib_user_cpnew;
        void  (*const alib_delete)(void* p, void(*dest_func)(void*)) = __alib_user_delete;
    #else
        void  (*const alib_free)(void* p) = __alib_default_free;
        void* (*const alib_alloc)(uint32_t size) = __alib_default_alloc;
        void* (*const alib_realloc)(void* p, uint32_t size) = __alib_default_realloc;
        void* (*const alib_new)(uint32_t size, void(*init_func)(void*)) = __alib_default_new;
        void* (*const alib_cpnew)(uint32_t size, const void* that, void(*copy_func)(void*, const void*)) = __alib_default_cpnew;
        void  (*const alib_delete)(void* p, void(*dest_func)(void*)) = __alib_default_delete;
    #endif /* __ALIB_USER_ALLOC__ */
#endif /* windows */

#if defined(__C_POSIX__)
__weak void  alib_free(void* p){
    __alib_default_free(p);
}
__weak void* alib_alloc(uint32_t size){
    return __alib_default_alloc(size);
}
__weak void* alib_realloc(void* p, uint32_t size){
    return __alib_default_realloc(p, size);
}
__weak void* alib_new(uint32_t size, void(*init_func)(void*)){
    return __alib_default_new(size, init_func);
}
__weak void* alib_cpnew(uint32_t size, const void* that, void(*copy_func)(void*, const void*)){
    return __alib_default_cpnew(size, that, copy_func);
}
__weak void  alib_delete(void* p, void(*dest_func)(void*)){
    __alib_default_delete(p, dest_func);
}
#endif /* posix */


/* hash function */
uint32_t alib_hash(const void* k, uint32_t size_k) {
    if(__a_unlikely(k == nullptr || size_k == 0)){
        return 0;
    }

    // FNV-a 32位算法常量
    const uint32_t FNV_prime = 16777619U;
    const uint32_t offset_basis = 2166136261U;

    uint32_t hv = offset_basis;

    const uint8_t* data = (const uint8_t*)k;
    for(uint32_t i = 0; i < size_k; i++){
        hv ^= data[i];
        hv *= FNV_prime;
    }

    // 扰动函数：混合高位与低位，改善分布（尤其当数组长度较小时）
    hv ^= (hv >> 16);

    return hv;
}

uint32_t alib_hash_str(const char* s) {
    if(__a_unlikely(s == nullptr)){
        return 0;
    }
    return alib_hash(s, strlen(s));
}



/* exception handling */
thread_local int __A_EXC_VALUE__ = 0;




