/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <aline.h>

/**************************************************************************************/
void __Aarr_dest(__Aarr* arr){
    alib_free(arr->data);
}

int __Aarr_add_cap_back(__Aarr* arr){
    uint32_t cap = 0;
    uint32_t obj_size = arr->size;

    if(arr->cap * obj_size >= __aPagSize){
        cap = arr->cap + __aPagSize / obj_size;
    }else{
        cap = arr->cap * 2;
    }

    if(__a_unlikely(cap < 16)){
        cap = 16;
    }

    char* p = alib_realloc(arr->data, cap * obj_size);
    if(__a_unlikely(p == nullptr)){
        return AERR_alloc_failed;
    }
    arr->data = p, arr->cap = cap;

    return 0;
}
int __Aarr_add_cap_front(__Aarr* arr){
    uint32_t cap = 0;
    uint32_t obj_size = arr->size;

    if(arr->cap * obj_size >= __aPagSize){
        cap = arr->cap + __aPagSize / obj_size;
    }else{
        cap = arr->cap * 2;
    }

    if(__a_unlikely(cap < 16)){
        cap = 16;
    }

    uint32_t new_offset = cap - arr->cap + arr->offset;

    char* p = alib_alloc(cap * obj_size);
    if(__a_unlikely(p == nullptr)){
        return AERR_alloc_failed;
    }

    char* src = arr->data != nullptr ? (arr->data + arr->offset * obj_size) : nullptr;
    char* tar = p + new_offset * obj_size;
    if(src != nullptr) memcpy(tar, src, arr->num * obj_size);

    alib_free(arr->data);
    arr->data = p, arr->cap = cap, arr->offset = new_offset;

    return 0;
}
void __Aarr_sub_cap_back(__Aarr* arr){
    uint32_t cap = 0;
    uint32_t obj_size = arr->size;
    uint32_t red = arr->cap - (arr->offset + arr->num);

    if(red * obj_size >= __aPagSize){
        cap = arr->cap - __aPagSize / obj_size;
    }else{
        cap = (arr->cap + 1) / 2;
    }

    if(__a_unlikely(cap < 16)){
        cap = 16;
    }

    char* p = alib_realloc(arr->data, cap * obj_size);
    if(__a_likely(p != nullptr)){
        arr->data = p, arr->cap = cap;
    }
}
void __Aarr_sub_cap_front(__Aarr* arr){
    uint32_t cap = 0;
    uint32_t obj_size = arr->size;

    char* src = arr->data + arr->offset * obj_size;
    char* tar = arr->data;
    memmove(tar, src, arr->num * obj_size);

    cap = arr->cap - arr->offset;
    if(__a_unlikely(cap < 16)){
        cap = 16;
    }


    char* p = alib_realloc(arr->data, cap * obj_size);
    if(__a_likely(p != nullptr)){
        arr->data = p, arr->cap = cap, arr->offset = 0;
    }
}


int __Aarr_copy(__Aarr* arr, const __Aarr* that_arr){
    memset(arr, 0, sizeof(__Aarr));
    arr->size = that_arr->size;

    uint32_t obj_size = that_arr->size;
    uint32_t num = that_arr->num;
    if(num == 0) return 0;

    char* p = alib_alloc(obj_size * num);
    if(__a_unlikely(p == nullptr)){
        return AERR_alloc_failed;
    }
    arr->data = p, arr->cap = num;

    memcpy(arr->data, that_arr->data, obj_size * num);

    return 0;
}
void* __Aarr_at(__Aarr* arr, uint32_t i){
    uint32_t obj_size = arr->size;
    return arr->data + obj_size * (arr->offset + i);
}
int __Aarr_ins(__Aarr* arr, uint32_t i){
    uint32_t obj_size = arr->size;

    if(i < arr->num / 2){
        uint32_t red = arr->offset;
        if(__a_unlikely(red == 0 && __Aarr_add_cap_front(arr) != 0)){
            return AERR_alloc_failed;
        }

        if(i != 0){
            char* src = __Aarr_at(arr, 0);
            char* tar = src - obj_size;
            memmove(tar, src, obj_size * i);
        }
        arr->num++, arr->offset--;
    }else{
        uint32_t red = arr->cap - (arr->offset + arr->num);
        if(__a_unlikely(red == 0 && __Aarr_add_cap_back(arr) != 0)){
            return AERR_alloc_failed;
        }

        if(i != arr->num){
            char* src = __Aarr_at(arr, i);
            char* tar = __Aarr_at(arr, i + 1);
            memmove(tar, src, obj_size * (arr->num - i));
        }
        arr->num++;
    }
    return 0;
}
void __Aarr_rm(__Aarr* arr, uint32_t i){
    uint32_t obj_size = arr->size;

    if(i < arr->num / 2){
        if(i != 0){
            char* src = __Aarr_at(arr, 0);
            char* tar = __Aarr_at(arr, 1);
            memmove(tar, src, obj_size * i);
        }
        arr->offset++, arr->num--;

        uint32_t red = arr->offset;
        if(__a_unlikely(arr->cap > 16 && (red * arr->size >= __aPagSize || red >= (arr->cap + 1) / 2))){
            __Aarr_sub_cap_front(arr);
        }
    }else{
        if(i != arr->num - 1){
            char* src = __Aarr_at(arr, i + 1);
            char* tar = __Aarr_at(arr, i);
            memmove(tar, src, obj_size * (arr->num - i - 1));
        }
        arr->num--;

        uint32_t red = arr->cap - (arr->offset + arr->num);
        if(__a_unlikely(arr->cap > 16 && (red * arr->size >= __aPagSize || red >= (arr->cap + 1) / 2))){
            __Aarr_sub_cap_back(arr);
        }
    }
}


