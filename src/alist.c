/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <alist.h>

static inline void __Alist_delNode(__Alist* list, __AlsNode* node, bool de){
    if(de) list->dest(__Alist_getObj(node));
    alib_free(node);
}

static inline __AlsNode* __Alist_newNode(__Alist* list, const void* src){
    __AlsNode* node = alib_alloc(list->size + sizeof(__AlsNode));
    if(__a_unlikely(node == nullptr)){
        return nullptr;
    }
    memset(node, 0, sizeof(__AlsNode));

    aErrClean();
    list->copy(__Alist_getObj(node), src);
    if(aErrOccur()){
        alib_free(node);
        node = nullptr;
    }

    return node;
}


void __Alist_dest(__Alist* list){
    __AlsNode* node = list->head;
    while(node != nullptr){
        __AlsNode* next = node->next;
        __Alist_delNode(list, node, true);
        node = next;
    }
    list->head = list->tail = nullptr;
    list->num = 0;
}

void __Alist_copy(__Alist* list, const __Alist* that_list){
    *list = *that_list;
    list->num = 0;
    list->head = list->tail = nullptr;

    int ret = 0;
    for(__AlsNode* node = that_list->head; ret == 0 && node != nullptr; node = node->next){
        ret = __Alist_pushBack(list, __Alist_getObj(node));
    }

    if(ret != 0) aErrSet(ret);
}

int __Alist_cmpd(const __Alist* list, const __Alist* that_list){
    int ret = 0;
    __AlsNode* node = list->head;
    __AlsNode* node_that = that_list->head;
    while(node != nullptr && node_that != nullptr){
        ret = list->cmpd(node->data, node_that->data);
        if(ret != 0) break;
        node = node->next;
        node_that = node_that->next;
    }
    if(ret == 0){
        ret = node == node_that ? 0 : (node_that == nullptr ? 1 : -1);
    }

    return ret;
}


static inline __AlsNode* __Alist_at_node(const __Alist* list, uint32_t index){
    if(__a_unlikely(index == AEND)) index = list->num - 1;
    if(__a_unlikely(index >= list->num)) return nullptr;

    __AlsNode* node = nullptr;
    if(index <= list->num / 2){
        node = list->head;
        for(uint32_t i = 0; i < index; i++){
            node = node->next;
        }
    }else{
        node = list->tail;
        for(uint32_t i = list->num - 1; i > index; i--){
            node = node->prev;
        }
    }
    return node;
}

void* __Alist_at(const __Alist* list, uint32_t index){
    return __Alist_getObj(__Alist_at_node(list, index));
}

int __Alist_ins(__Alist* list, uint32_t index, const void* source){
    if(__a_unlikely(index > list->num)) index = list->num;

    if(index == 0){
        __Alist_pushFront(list, source);
    }else if(index == list->num){
        __Alist_pushBack(list, source);
    }else{
        __AlsNode* next = __Alist_at_node(list, index);
        __AlsNode* prev = next != nullptr ? next->prev : nullptr;
        if(__a_unlikely(next == nullptr)){
            aErrSet(AERR_overstep);
            return AERR_overstep;
        }

        __AlsNode* node = __Alist_newNode(list, source);
        if(__a_unlikely(node == nullptr)){
            aErrSet(AERR_alloc_failed);
            return AERR_alloc_failed;
        }

        node->next = next, node->prev = prev;
        prev->next = node, next->prev = node;
        list->num++;
    }
    return 0;
}
int __Alist_pushFront(__Alist* list, const void* source){
    __AlsNode* node = __Alist_newNode(list, source);
    if(__a_unlikely(node == nullptr)){
        aErrSet(AERR_alloc_failed);
        return AERR_alloc_failed;
    }

    if(__a_unlikely(list->num == 0)){
        list->head = list->tail = node;
    }else{
        list->head->prev = node;
        node->next = list->head;
        list->head = node;
    }

    list->num++;
    return 0;
}

int __Alist_pushBack(__Alist* list, const void* source){
    __AlsNode* node = __Alist_newNode(list, source);
    if(__a_unlikely(node == nullptr)){
        aErrSet(AERR_alloc_failed);
        return AERR_alloc_failed;
    }

    if(__a_unlikely(list->num == 0)){
        list->head = list->tail = node;
    }else{
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }

    list->num++;
    return 0;
}

void __Alist_rm_node(__Alist* list, __AlsNode* node, bool de){
    if(__a_unlikely(node == nullptr)){
        aErrSet(AERR_overstep);
        return;
    }

    if(node->prev != nullptr){
        node->prev->next = node->next;
    }
    if(node->next != nullptr){
        node->next->prev = node->prev;
    }

    if(node == list->head){
        list->head = node->next;
    }
    if(node == list->tail){
        list->tail = node->prev;
    }

    list->num--;
    __Alist_delNode(list, node, de);
}

void __Alist_rm(__Alist* list, uint32_t index){
    __AlsNode* node = __Alist_at_node(list, index);
    if(__a_likely(node != nullptr)){
        __Alist_rm_node(list, node, true);
    }
}

void __Alist_popBack(__Alist* list, void* tar){
    if(tar != nullptr) memset(tar, 0, list->size);

    if(__a_unlikely(list->num == 0)){
        aErrSet(AERR_overstep);
        return;
    }

    if(tar == nullptr){
        __Alist_rm(list, list->num - 1);
        return;
    }

    __AlsNode* node = list->tail;
    if(__a_likely(node != nullptr)){
        memcpy(tar, __Alist_getObj(node), list->size);
        __Alist_rm_node(list, node, false);
    }else{
        aErrSet(AERR_overstep);
    }
}

void __Alist_popFront(__Alist* list, void* tar){
    if(tar != nullptr) memset(tar, 0, list->size);

    if(__a_unlikely(list->num == 0)){
        aErrSet(AERR_overstep);
        return;
    }

    if(tar == nullptr){
        __Alist_rm(list, 0);
        return;
    }

    __AlsNode* node = list->head;
    if(__a_likely(node != nullptr)){
        memcpy(tar, __Alist_getObj(node), list->size);
        __Alist_rm_node(list, node, false);
    }else{
        aErrSet(AERR_overstep);
    }
}

void __Alist_take(__Alist* list, uint32_t index, void* tar){
    if(tar != nullptr) memset(tar, 0, list->size);

    if(__a_unlikely(list->num == 0)){
        aErrSet(AERR_overstep);
        return;
    }
    if(tar == nullptr){
        __Alist_rm(list, index);
        return;
    }
    __AlsNode* node = __Alist_at_node(list, index);
    if(__a_likely(node != nullptr)){
        memcpy(tar, __Alist_getObj(node), list->size);
        __Alist_rm_node(list, node, false);
    }else{
        aErrSet(AERR_overstep);
    }
}

void __Alist_take_node(__Alist* list, __AlsNode* node, void* tar){
    if(tar != nullptr) memset(tar, 0, list->size);

    if(__a_unlikely(node == nullptr)){
        aErrSet(AERR_overstep);
        return;
    }

    if(__a_unlikely(list->num == 0)){
        aErrSet(AERR_overstep);
        return;
    }
    if(tar == nullptr){
        __Alist_rm_node(list, node, true);
        return;
    }else{
        memcpy(tar, __Alist_getObj(node), list->size);
        __Alist_rm_node(list, node, false);
    }
}
