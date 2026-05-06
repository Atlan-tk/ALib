/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <alib.h>
#include <stddef.h>
#include <stdlib.h>

/* __a_ref_count */
/* Keep the payload naturally aligned even after the ref-count header. */
typedef union {
    atomic_size_t ref_count;
    max_align_t payload_align;
} __a_ref_count_head;

/* 初始化计数为1 */
static inline void __a_ref_count_set(__a_ref_count_head* ref_head){
    atomic_store_explicit(&ref_head->ref_count, 1, memory_order_relaxed);
}
__unused static inline bool __a_ref_count_valid(__a_ref_count_head* ref_head){
    return atomic_load_explicit(&ref_head->ref_count, memory_order_relaxed) >= 1;
}
/* 返回为真则自增成功 */
static inline bool __a_ref_count_add(__a_ref_count_head* ref_head){
    if(__a_unlikely(atomic_fetch_add(&ref_head->ref_count, 1) > 0)){
        return true;
    }else{
        atomic_store(&ref_head->ref_count, 0);
    }
    return false;
}
/* 返回为真则可释放 */
static inline bool __a_ref_count_sub(__a_ref_count_head* ref_head){
    if(__a_unlikely(atomic_fetch_sub(&ref_head->ref_count, 1) == 1)){
        return true;
    }
    return false;
}



/* memory management */
__unused __weak void  alib_free(void* p){ free(p); };
__unused __weak void* alib_alloc(uint32_t size){ return malloc(size); };
__unused __weak void* alib_realloc(void* p, uint32_t size){
    return realloc(p, size);
}
__unused __weak void* alib_new(uint32_t size, void(*init_func)(void*)){
    aExcClean();
    void* p = alib_alloc(size);
    if(__a_likely(p != nullptr && init_func != nullptr)){
        init_func(p);
        if(aExcOccur()){
            alib_free(p); p = nullptr;
        }
    }
    return p;
}
__unused void* alib_new_for_copy(uint32_t size, const void* that, void(*copy_func)(void*, const void*)){
    aExcClean();
    void* p = alib_alloc(size);
    if(__a_likely(p != nullptr && copy_func != nullptr)){
        copy_func(p, that);
        if(aExcOccur()){
            alib_free(p); p = nullptr;
        }
    }
    return p;
}
__unused __weak void  alib_delete(void* p, void(*dest_func)(void*)){
    if(__a_likely(p != nullptr)){
        if(__a_likely(dest_func != nullptr)) dest_func(p);
        alib_free(p);
    }
}

__unused __weak void* alib_ref_new(uint32_t size, void(*init_func)(void*)){
    aExcClean();
    __a_ref_count_head* ref_head = alib_alloc(size + sizeof(__a_ref_count_head));
    if(__a_likely(ref_head != nullptr)){
        void* p = ref_head + 1;
        __a_ref_count_set(ref_head);
        if(__a_likely(init_func != nullptr)) init_func(p);
        if(aExcOccur()){
            alib_free(ref_head); p = nullptr;
        }
        return p;
    }

    return nullptr;
}
__unused void* alib_ref_new_for_copy(uint32_t size, const void* that, void(*copy_func)(void*, const void*)){
    aExcClean();
    __a_ref_count_head* ref_head = alib_alloc(size + sizeof(__a_ref_count_head));
    if(__a_likely(ref_head != nullptr)){
        void* p = ref_head + 1;
        __a_ref_count_set(ref_head);
        if(__a_likely(copy_func != nullptr)) copy_func(p, that);
        if(aExcOccur()){
            alib_free(ref_head); p = nullptr;
        }
        return p;
    }

    return nullptr;
}
__unused __weak void  alib_ref_delete(void* p, void(*dest_func)(void*)){
    if(__a_likely(p != nullptr)){
        __a_ref_count_head* ref_head = p; ref_head--;
        if(__a_ref_count_sub(ref_head)){
            if(__a_likely(dest_func != nullptr)) dest_func(p);
            alib_free(ref_head);
        }
    }
};
__unused __weak void* alib_ref_copy(void* p){
    if(__a_likely(p != nullptr)){
        __a_ref_count_head* ref_head = p; ref_head--;
        if(__a_ref_count_add(ref_head)){
            return p;
        }
    }

    return nullptr;
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
