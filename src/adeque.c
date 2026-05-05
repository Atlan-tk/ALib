/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <adeque.h>

void __A2arr_init(__A2arr* arr, uint32_t size){
    arr->size = size;
    arr->blk_size = __aPagSize / size;
    if(arr->blk_size < 16) arr->blk_size = 16;
}

void __A2arr_dest(__A2arr* arr){
    for(uint32_t i = 0; i < arr->num; i++){
        char* p = arr->data[i + arr->offset];
        alib_free(p);
    }
    alib_free(arr->data);
}

int __A2arr_add_cap_back(__A2arr* arr){
    uint32_t cap = 0;
    uint32_t obj_size = sizeof(char*);

    if(arr->cap * obj_size >= __aPagSize){
        cap = arr->cap + __aPagSize / obj_size;
    }else{
        cap = arr->cap * 2;
    }

    if(__a_unlikely(cap < 16)){
        cap = 16;
    }

    char** p = alib_realloc(arr->data, cap * obj_size);
    if(__a_unlikely(p == nullptr)){
        return AEXC_alloc_failed;
    }
    arr->data = p, arr->cap = cap;

    return 0;
}
int __A2arr_add_cap_front(__A2arr* arr){
    uint32_t cap = 0;
    uint32_t obj_size = sizeof(char*);

    if(arr->cap * obj_size >= __aPagSize){
        cap = arr->cap + __aPagSize / obj_size;
    }else{
        cap = arr->cap * 2;
    }

    if(__a_unlikely(cap < 16)){
        cap = 16;
    }

    uint32_t new_offset = cap - arr->cap + arr->offset;

    char** p = alib_alloc(cap * obj_size);
    if(__a_unlikely(p == nullptr)){
        return AEXC_alloc_failed;
    }

    char** src = arr->data + arr->offset;
    char** tar = p + new_offset;
    memcpy(tar, src, arr->num * obj_size);

    alib_free(arr->data);
    arr->data = p, arr->cap = cap, arr->offset = new_offset;

    return 0;
}
int __A2arr_sub_cap_back(__A2arr* arr){
    if(__a_unlikely(arr->cap == 0)){
        return AEXC_overstep;
    }

    uint32_t cap = 0;
    uint32_t obj_size = sizeof(char*);
    uint32_t red = arr->cap - (arr->offset + arr->num);

    if(red * obj_size >= __aPagSize){
        cap = arr->cap - __aPagSize / obj_size;
    }else{
        cap = (arr->cap + 1) / 2;
    }

    if(__a_unlikely(cap < 16)){
        cap = 16;
    }

    char** p = alib_realloc(arr->data, cap * obj_size);
    if(__a_unlikely(p == nullptr)){
        return AEXC_alloc_failed;
    }
    arr->data = p, arr->cap = cap;

    return 0;
}
int __A2arr_sub_cap_front(__A2arr* arr){
    if(__a_unlikely(arr->cap == 0)){
        return AEXC_overstep;
    }

    uint32_t cap = 0;
    uint32_t obj_size = sizeof(char*);

    char** src = arr->data + arr->offset;
    char** tar = arr->data;
    memmove(tar, src, arr->num * obj_size);

    cap = arr->cap - arr->offset;
    if(__a_unlikely(cap < 16)){
        cap = 16;
    }


    char** p = alib_realloc(arr->data, cap * obj_size);
    if(__a_unlikely(p == nullptr)){
        return AEXC_alloc_failed;
    }
    arr->data = p, arr->cap = cap, arr->offset = 0;

    return 0;
}

int __A2arr_add_blk_back (__A2arr* arr){
    uint32_t red = arr->cap - (arr->offset + arr->num);
    if(__a_unlikely(red == 0 && __A2arr_add_cap_back(arr) != 0)){
        return AEXC_alloc_failed;
    }

    char* p = alib_alloc(arr->size * arr->blk_size);
    if(p == nullptr){
        return AEXC_alloc_failed;
    }

    arr->num++;
    arr->data[arr->num - 1 + arr->offset] = p;
    return 0;
}
int __A2arr_add_blk_front(__A2arr* arr){
    uint32_t red = arr->offset;
    if(__a_unlikely(red == 0 && __A2arr_add_cap_front(arr) != 0)){
        return AEXC_alloc_failed;
    }

    char* p = alib_alloc(arr->size * arr->blk_size);
    if(p == nullptr){
        return AEXC_alloc_failed;
    }

    arr->offset--, arr->num++;
    arr->data[arr->offset] = p;
    return 0;
}
int __A2arr_sub_blk_back (__A2arr* arr){
    char* p = arr->data[arr->num - 1 + arr->offset];

    uint32_t red = arr->cap - (arr->offset + arr->num - 1);
    if(__a_unlikely(arr->cap > 16 && (red * arr->size >= __aPagSize || red >= (arr->cap + 1) / 2))){
        int ret = __A2arr_sub_cap_back(arr);
        if(__a_unlikely(ret != 0)){
            /* 失败意味着什么也不做 */
            return ret;
        }
    }

    arr->num--, alib_free(p);
    return 0;
}
int __A2arr_sub_blk_front(__A2arr* arr){
    char* p = arr->data[arr->offset];

    arr->offset++;
    uint32_t red = arr->offset;
    if(__a_unlikely(arr->cap > 16 && (red * arr->size >= __aPagSize || red >= (arr->cap + 1) / 2))){
        int ret = __A2arr_sub_cap_front(arr);
        if(__a_unlikely(ret != 0)){
            /* 失败意味着什么也不做 */
            arr->offset--;
            return ret;
        }
    }

    arr->num--, alib_free(p);
    return 0;
}





