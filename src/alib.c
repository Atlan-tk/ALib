/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <alib.h>
#include <stddef.h>
#include <stdlib.h>

/* memory management */
__unused __weak void  alib_free(void* p){ free(p); };
__unused __weak void* alib_alloc(uint32_t size){ return malloc(size); };
__unused __weak void* alib_realloc(void* p, uint32_t size){
    return realloc(p, size);
}
__unused __weak void* alib_new(uint32_t size, void(*init_func)(void*)){
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
__unused void* alib_cpnew(uint32_t size, const void* that, void(*copy_func)(void*, const void*)){
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
__unused __weak void  alib_delete(void* p, void(*dest_func)(void*)){
    if(__a_likely(p != nullptr)){
        if(__a_likely(dest_func != nullptr)) dest_func(p);
        alib_free(p);
    }
}



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

uint32_t A_OBJ_HASH(cstr_t)(const cstr_t* self){
    if(__a_unlikely(self == nullptr || *self == nullptr)){
        return 0;
    }
    return alib_hash_str(*self);
}

uint32_t A_OBJ_HASH(astr_t)(const astr_t* self){
    if(__a_unlikely(self == nullptr || self->s == nullptr)){
        return 0;
    }
    return alib_hash_str(self->s);
}



/* exception handling */
thread_local int __A_EXC_VALUE__ = 0;
