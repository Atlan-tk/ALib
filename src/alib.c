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

static void* __alib_default_new(uint32_t size, bool(*init_func)(void*)){
    aErrClean();
    void* p = alib_alloc(size);

    if(__a_unlikely(p == nullptr)){
        if(__a_likely(size != 0))
            aErrSet(AERR_alloc_failed);
        return nullptr;
    }

    bool ret = true;
    if(init_func != nullptr){
        ret = init_func(p);
    }else{
        memset(p, 0, size);
    }

    if(!ret){
        alib_free(p); p = nullptr;
    }

    return p;
}

static void* __alib_default_cpnew(uint32_t size, const void* that, bool(*copy_func)(void*, const void*)){
    aErrClean();
    void* p = alib_alloc(size);

    if(__a_unlikely(p == nullptr)){
        if(__a_likely(size != 0))
            aErrSet(AERR_alloc_failed);
        return nullptr;
    }

    bool ret = true;
    if(copy_func != nullptr){
        ret = copy_func(p, that);
    }else{
        memset(p, 0, size);
    }

    if(!ret){
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
        void* __alib_user_new(uint32_t size, bool(*init_func)(void*));
        void* __alib_user_cpnew(uint32_t size, const void* that, bool(*copy_func)(void*, const void*));
        void  __alib_user_delete(void* p, void(*dest_func)(void*));

        void  (*const alib_free)(void* p) = __alib_user_free;
        void* (*const alib_alloc)(uint32_t size) = __alib_user_alloc;
        void* (*const alib_realloc)(void* p, uint32_t size) = __alib_user_realloc;
        void* (*const alib_new)(uint32_t size, bool(*init_func)(void*)) = __alib_user_new;
        void* (*const alib_cpnew)(uint32_t size, const void* that, bool(*copy_func)(void*, const void*)) = __alib_user_cpnew;
        void  (*const alib_delete)(void* p, void(*dest_func)(void*)) = __alib_user_delete;
    #else
        void  (*const alib_free)(void* p) = __alib_default_free;
        void* (*const alib_alloc)(uint32_t size) = __alib_default_alloc;
        void* (*const alib_realloc)(void* p, uint32_t size) = __alib_default_realloc;
        void* (*const alib_new)(uint32_t size, bool(*init_func)(void*)) = __alib_default_new;
        void* (*const alib_cpnew)(uint32_t size, const void* that, bool(*copy_func)(void*, const void*)) = __alib_default_cpnew;
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
__weak void* alib_new(uint32_t size, bool(*init_func)(void*)){
    return __alib_default_new(size, init_func);
}
__weak void* alib_cpnew(uint32_t size, const void* that, bool(*copy_func)(void*, const void*)){
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
uint32_t A_OBJ_HASH(void)(__noused const void* self){ return 0; };
uint32_t A_OBJ_HASH(Atlan)(__noused const Atlan* self){ return 0; }



thread_local int __a_err_value__ = 0;



bool a_fs_start(void);
bool a_mstimer_start(void);
bool a_signal_system_start(void);

void a_fs_poweroff(void);
void a_mstimer_poweroff(void);
void a_signal_system_poweroff(void);

__attribute__((destructor)) static inline void alib_poweroff(void){
    a_fs_poweroff();
    a_mstimer_poweroff();
    a_signal_system_poweroff();
}
__attribute__((constructor)) static inline void alib_start(void){
    bool ret = a_signal_system_start();
    if(ret) ret = a_mstimer_start();
    if(ret) ret = a_fs_start();

    if(!ret){
        alib_poweroff();
    }
}


