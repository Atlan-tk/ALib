/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <ahash.h>

//////////////////////////////////////////////////////////////
static uint32_t hash_func(const __Ahash* hash, const void* k, uint32_t arr_len){
    if(__a_unlikely(hash == nullptr || k == nullptr)){
        return 0;
    }

    uint32_t hv = 0;
    uint32_t size_k = hash->size_k;

    if(hash->hash_func != nullptr){
        hv = hash->hash_func(k);
    }else{
        hv = alib_hash(k, size_k);
    }

    if(__a_unlikely(hv == 0 || arr_len == 0)){
        return 0;
    }

    return hv % arr_len;
}

//////////////////////////////////////////////////////////////
static inline char* __AhsBucket_at(__AhsBucket* bt, uint32_t ele_size, uint32_t i){
    if(__a_unlikely(i >= bt->num)){
        return nullptr;
    }else{
        return bt->p + i * ele_size;
    }
}
static inline uint32_t __AhsBucket_find(__AhsBucket* bt, uint32_t ele_size,
        const void* k, int(cmpd)(const void*, const void*)){
    if(__a_unlikely(bt == 0)){
        return 0xffffffff;
    }

    for(uint32_t i = 0; i < bt->num; i++){
        char* element = bt->p + i * ele_size;
        if(cmpd(element, k) == 0){
            return i;
        }
    }

    return 0xffffffff;
}
static inline int __AhsBucket_expand(__AhsBucket* bt, uint32_t ele_size){
    if(bt->num >= bt->cap){
        uint32_t cap = bt->cap * 2;
        if(cap == 0) cap = 8;

        char* p = alib_realloc(bt->p, ele_size * cap);
        if(__a_unlikely(p == nullptr)){
            return AEXC_alloc_failed;
        }
        bt->p = p, bt->cap = cap;
    }

    return 0;
}
static inline char* __AhsBucket_push(__AhsBucket* bt, uint32_t ele_size){
    if(__AhsBucket_expand(bt, ele_size) != 0){
        return nullptr;
    }
    bt->num++;
    return __AhsBucket_at(bt, ele_size, bt->num - 1);
}
static inline void __AhsBucket_rm(__AhsBucket* bt, uint32_t ele_size, uint32_t i){
    if(__a_unlikely(i > bt->num)){
        return;
    }

    char* tar = bt->p + i * ele_size;
    memmove(tar, tar + ele_size, ele_size * (bt->num - i - 1));
    bt->num--;
}
static inline void __AhsBucket_dest(__AhsBucket* bt, uint32_t ele_size, void(*dest)(void*)){
    for(uint32_t i = 0; i < bt->num; i++){
        char* element = __AhsBucket_at(bt, ele_size, i);
        dest(element);
    }
    alib_free(bt->p);
}

//////////////////////////////////////////////////////////////
static inline void __Ahash_freeTab(__AhsBucket* arr, uint32_t bucket_num){
    for(uint32_t i = 0; i < bucket_num; i++){
        alib_free(arr[i].p);
    }
    alib_free(arr);
}

//////////////////////////////////////////////////////////////
static inline __AhsBucket* __Ahash_at_bt(const __Ahash* hash, uint32_t i){
    if(__a_unlikely(i >= hash->bucket_num)){
        return nullptr;
    }
    return hash->bucket_arr + i;
}
static inline void __Ahash_dest_arr(__Ahash* hash){
    for(uint32_t i = 0; i < hash->bucket_num; i++){
        __AhsBucket* bt = __Ahash_at_bt(hash, i);
        __AhsBucket_dest(bt, hash->size, hash->dest);
    }
    alib_free(hash->bucket_arr);
}
static inline int __Ahash_reHash(__Ahash* hash, uint32_t cap){
    if(__a_unlikely(cap < 16)){
        cap = 16;
    }

    __AhsBucket* arr = alib_alloc(cap * sizeof(__AhsBucket));
    if(__a_unlikely(arr == nullptr)){
        return AEXC_alloc_failed;
    }
    memset(arr, 0, cap * sizeof(__AhsBucket));

    if(hash->num == 0){
        hash->bucket_arr = arr;
        hash->bucket_num = cap;
        return 0;
    }

    for(uint32_t i = 0; i < hash->bucket_num; i++){
        __AhsBucket* bt = __Ahash_at_bt(hash, i);

        for(uint32_t j = 0; j < bt->num; j++ ){
            char* element = __AhsBucket_at(bt, hash->size, j);

            uint32_t hv = hash_func(hash, element, cap);
            __AhsBucket* new_bt = &arr[hv];
            char* new_element = __AhsBucket_push(new_bt, hash->size);
            if(new_element == nullptr){
                __Ahash_freeTab(arr, cap);
                return AEXC_alloc_failed;
            }

            memcpy(new_element, element, hash->size);
        }
    }

    __Ahash_freeTab(hash->bucket_arr, hash->bucket_num);
    hash->bucket_arr = arr;
    hash->bucket_num = cap;
    return 0;
}
static inline int __Ahash_reHash_up(__Ahash* hash){
    uint32_t cap = hash->bucket_num * 2;
    return __Ahash_reHash(hash, cap);
}
static inline int __Ahash_reHash_down(__Ahash* hash){
    uint32_t cap = (hash->bucket_num + 1) / 2;
    return __Ahash_reHash(hash, cap);
}
static inline int __Ahash_add(__Ahash* hash, void* obj){
    uint32_t obj_size = hash->size;
    uint32_t hv = hash_func(hash, obj, hash->bucket_num);
    __AhsBucket* bt = __Ahash_at_bt(hash, hv);

    uint32_t y = __AhsBucket_find(bt, obj_size, obj, hash->cmpd_k);

    if(y != 0xffffffff){
        //已存在
        char* element = __AhsBucket_at(bt, obj_size, y);
        if(__a_likely(element != nullptr)){
            char buf[obj_size]; memset(buf, 0, obj_size);
            memcpy(buf, element, obj_size);
            hash->copy(element, obj);
            if(aExcOccur()){
                memcpy(element, buf, obj_size);
                return aExcGet();
            }else{
                hash->dest(buf);
                return 0;
            }
        }
    }

    //新添加
    char* element = __AhsBucket_push(bt, obj_size);
    if(__a_unlikely(element == nullptr)){
        return AEXC_alloc_failed;
    }
    hash->copy(element, obj);
    if(aExcOccur()){
        __AhsBucket_rm(bt, obj_size, bt->num - 1);
        return aExcGet();
    }else{
        hash->num++;
        return 0;
    }
}
static inline int __Ahash_del(__Ahash* hash, const void* k){
    uint32_t obj_size = hash->size;
    uint32_t hv = hash_func(hash, k, hash->bucket_num);
    __AhsBucket* bt = __Ahash_at_bt(hash, hv);

    if(__a_unlikely(hash->num == 0)){
        return AEXC_overstep;
    }

    uint32_t y = __AhsBucket_find(bt, obj_size, k, hash->cmpd_k);
    if(y == 0xffffffff){
        //不存在
        return AEXC_overstep;
    }else{
        char* element = __AhsBucket_at(bt, obj_size, y);
        if(__a_unlikely(element == nullptr)){
            return AEXC_overstep;
        }

        hash->dest(element);
        hash->num--;
        __AhsBucket_rm(bt, obj_size, y);
        return 0;
    }
    return 0;
}
static inline int __Ahash_reCap(__Ahash* hash){
    if(__a_unlikely(hash->bucket_num == 0 || hash->num * 2 / 3 >= hash->bucket_num)){
        return __Ahash_reHash_up(hash);
    }
    if(__a_unlikely(hash->bucket_num != 0 && hash->num * 3 <= hash->bucket_num)){
        return __Ahash_reHash_down(hash);
    }
    return 0;
}
void __Ahash_dest(__Ahash* hash){
    __Ahash_dest_arr(hash);
}
void* __Ahash_at(const __Ahash* hash, const void* k){
    uint32_t obj_size = hash->size;

    uint32_t hv = hash_func(hash, k, hash->bucket_num);
    __AhsBucket* bt = __Ahash_at_bt(hash, hv);

    uint32_t y = __AhsBucket_find(bt, obj_size, k, hash->cmpd_k);
    if(__a_unlikely(y == 0xffffffff)){
        return nullptr;
    }

    char* element = __AhsBucket_at(bt, obj_size, y);
    if(__a_unlikely(element == nullptr)){
        return nullptr;
    }

    return element;
}
void __Ahash_rm(__Ahash* hash, const void* k){
    if(__a_unlikely(__Ahash_del(hash, k) != 0)){
        aExcSet(AEXC_overstep);
    }
}
void __Ahash_ins(__Ahash* hash, void* data){
    int ret = __Ahash_reCap(hash);
    if(__a_unlikely(ret != 0)){
        aExcSet(ret);
        return;
    }

    ret = __Ahash_add(hash, data);

    if(__a_unlikely(ret != 0)){
        aExcSet(ret);
    }
}

void __Ahash_take(__Ahash* hash, void* data){
    const void* k = data;

    uint32_t obj_size = hash->size;
    uint32_t hv = hash_func(hash, k, hash->bucket_num);
    __AhsBucket* bt = __Ahash_at_bt(hash, hv);

    if(__a_unlikely(hash->num == 0)){
        aExcSet(AEXC_overstep);
        return;
    }

    uint32_t y = __AhsBucket_find(bt, obj_size, k, hash->cmpd_k);
    if(y == 0xffffffff){
        //不存在
        aExcSet(AEXC_overstep);
        return;
    }else{
        char* element = __AhsBucket_at(bt, obj_size, y);
        if(__a_unlikely(element == nullptr)){
            aExcSet(AEXC_overstep);
            return;
        }

        memcpy(data, element, hash->size);
        __AhsBucket_rm(bt, obj_size, y);
        hash->num--;
    }
}
void __Ahash_get_head(const __Ahash* hash, __Aiter* it){
    if(hash->num == 0) return;

    for(uint32_t i = 0; i < hash->bucket_num; i++){
        uint32_t x = i;
        __AhsBucket* bt = __Ahash_at_bt(hash, x);
        if(bt->num != 0){
            char* element = __AhsBucket_at(bt, hash->size, 0);
            it->p = element;
            it->r = x;
            it->i = 0;
            return;
        }
    }
}
void __Ahash_get_tail(const __Ahash* hash, __Aiter* it){
    if(hash->num == 0) return;

    for(uint32_t i = 0; i < hash->bucket_num; i++){
        uint32_t x = hash->bucket_num - i - 1;
        __AhsBucket* bt = __Ahash_at_bt(hash, x);
        if(bt->num != 0){
            char* element = __AhsBucket_at(bt, hash->size, bt->num - 1);
            it->p = element;
            it->r = x;
            it->i = hash->num - 1;
            return;
        }
    }
}
void __Ahash_iter_next(const __Ahash* hash, __Aiter* it){
    uint32_t ele_size = hash->size;

    char* element = it->p;
    if(__a_unlikely(element == nullptr)){
        return;
    }

    __AhsBucket* bt = __Ahash_at_bt(hash, it->r);
    uint32_t offset = (element - bt->p) / ele_size;

    if(offset != bt->num - 1){
        element = __AhsBucket_at(bt, ele_size, offset + 1);
        it->p = element;
        it->i++;
        return;
    }

    for(uint32_t i = it->r + 1; i < hash->bucket_num; i++){
        uint32_t x = i;

        __AhsBucket* bt = __Ahash_at_bt(hash, x);
        if(bt->num != 0){
            char* element = __AhsBucket_at(bt, hash->size, 0);
            it->p = element;
            it->r = x;
            it->i++;
            return;
        }
    }

    it->i++;
}
void __Ahash_iter_prev(const __Ahash* hash, __Aiter* it){
    uint32_t ele_size = hash->size;

    char* element = it->p;
    if(__a_unlikely(element == nullptr)){
        return;
    }

    __AhsBucket* bt = __Ahash_at_bt(hash, it->r);
    uint32_t offset = (element - bt->p) / ele_size;

    if(offset != 0){
        element = __AhsBucket_at(bt, ele_size, offset - 1);
        it->p = element;
        it->i--;
        return;
    }

    uint32_t bucket_num = it->r;
    for(uint32_t i = 0; i < bucket_num; i++){
        uint32_t x = bucket_num - i - 1;

        __AhsBucket* bt = __Ahash_at_bt(hash, x);
        if(bt->num != 0){
            char* element = __AhsBucket_at(bt, hash->size, bt->num - 1);
            it->p = element;
            it->r = x;
            it->i--;
            return;
        }
    }

    it->i--;
}

static inline bool __Aiter_exist(const __Ahash* hash, __Aiter* it){
    if(hash->num == 0 || it->i >= hash->num){
        return false;
    }
    return true;
}
int __Ahash_cmpd(const __Ahash* self, const __Ahash* that){
    int ret = 0;

    __Aiter it_self = {}; __Ahash_get_head(self, &it_self);
    while(__Aiter_exist(self, &it_self)){
        char* obj_self = (char*)(it_self.p);
        char* obj_that = __Ahash_at(that, obj_self);
        ret = self->cmpd(obj_self, obj_that);
        if(ret != 0) return ret;

        __Ahash_iter_next(self, &it_self);
    }

    if(self->num == that->num) return 0;
    if(self->num > that->num) return 1;
    if(self->num < that->num) return -1;

    return 0;
}

int __Ahash_copy(__Ahash* self, const __Ahash* that){
    *self = *that;
    self->num = 0;
    self->bucket_num = 0;
    self->bucket_arr = nullptr;

    if(that->num == 0){
        return 0;
    }

    uint32_t cap = that->bucket_num;
    if(__a_unlikely(cap == 0)){
        cap = 16;
    }

    __AhsBucket* arr = alib_alloc(cap * sizeof(__AhsBucket));
    if(__a_unlikely(arr == nullptr)){
        return AEXC_alloc_failed;
    }
    memset(arr, 0, cap * sizeof(__AhsBucket));
    self->bucket_arr = arr;
    self->bucket_num = cap;

    for(uint32_t i = 0; i < that->bucket_num; i++){
        __AhsBucket* bt_self = __Ahash_at_bt(self, i);
        __AhsBucket* bt_that = __Ahash_at_bt(that, i);

        for(uint32_t j = 0; j < bt_that->num; j++ ){
            char* element_that = __AhsBucket_at(bt_that, self->size, j);
            char* element_self = __AhsBucket_push(bt_self, self->size);
            if(element_self == nullptr){
                return AEXC_alloc_failed;
            }

            self->copy(element_self, element_that);
            if(aExcOccur()){
                __AhsBucket_rm(bt_self, self->size, bt_self->num - 1);
                return aExcGet();
            }
            self->num++;
        }
    }

    return 0;
}



