/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __alist_h__
#define __alist_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"

typedef struct __AlsNode __AlsNode;
struct __AlsNode{
    __AlsNode *prev, *next;
    char data[0];
};

typedef struct{
    __AlsNode* head;
    __AlsNode* tail;
    uint32_t num;
    uint32_t size;
    void(*dest)(void*);
    int (*copy)(void*, const void*);
    int (*cmpd)(const void*, const void*);
}__Alist;

void __Alist_dest(__Alist* list);
void __Alist_copy(__Alist* list, const __Alist* that_list);
int __Alist_cmpd(const __Alist* list, const __Alist* that_list);

void*     __Alist_at          (const __Alist* list, uint32_t index);
void      __Alist_rm          (__Alist* list, uint32_t index);
void      __Alist_rm_node     (__Alist* list, __AlsNode* node, bool de);
int       __Alist_ins         (__Alist* list, uint32_t index, const void* obj);
int       __Alist_pushBack    (__Alist* list, const void* obj);
int       __Alist_pushFront   (__Alist* list, const void* obj);
void      __Alist_popBack     (__Alist* list, void* tar);
void      __Alist_popFront    (__Alist* list, void* tar);
void      __Alist_take        (__Alist* list, uint32_t index, void* tar);
void      __Alist_take_node   (__Alist* list, __AlsNode* node, void* tar);



static inline __AlsNode* __Alist_getNode(const void* d){
    return d != nullptr ? container_of(d, __AlsNode, data) : nullptr;
}
static inline void* __Alist_getObj(__AlsNode* node){
    return node != nullptr ? node->data : nullptr;
}


#define AList(T) __A_Splice(__A_Generic_List_$__, T)
#define __Alsf(T, name) __A_Splice(__A_Splice(__A_Splice(__A_Generic_List_$__, T), __func_$__), name)

#define AList_Define(T)                                                                     \
    typedef struct AList(T) AList(T);                                                       \
    typedef struct A_FUNC(AList(T)) A_FUNC(AList(T));                                       \
    struct AList(T){                                                                        \
        const A_FUNC(AList(T))* f;                                                          \
        __Alist list;                                                                       \
        T* type[0];                                                                         \
    };                                                                                      \
                                                                                            \
                                                                                            \
    AIter_Define(AList(T));                                                                 \
                                                                                            \
    struct A_FUNC(AList(T)){                                                                \
        bool    flag;                                                                       \
        void    (*dest)(void*);                                                             \
        T*      (*const at)       (const AList(T)* self, uint32_t index);                   \
        void    (*const rm)       (AList(T)* self, uint32_t index);                         \
        void    (*const rm_p)     (AList(T)* self, T* p);                                   \
        void    (*const ins)      (AList(T)* self, uint32_t index, const T obj);            \
        void    (*const take)     (AList(T)* self, uint32_t index, T* tar);                 \
        void    (*const take_p)     (AList(T)* self, T* p, T* tar);                         \
        void    (*const pushBack) (AList(T)* self, const T obj);                            \
        void    (*const pushFront)(AList(T)* self, const T obj);                            \
        void    (*const popBack)  (AList(T)* self, T* tar);                                 \
        void    (*const popFront) (AList(T)* self, T* tar);                                 \
        uint32_t(*const getNumber)(const AList(T)* self);                                   \
        bool    (*const empty)    (const AList(T)* self);                                   \
        AIter(AList(T)) (*const head)(const AList(T)* self);                                \
        AIter(AList(T)) (*const tail)(const AList(T)* self);                                \
        void    (*const next)(AIter(AList(T))* it);                                         \
        void    (*const prev)(AIter(AList(T))* it);                                         \
    };                                                                                      \
                                                                                            \
    static inline T* __Alsf(T,at)(const AList(T)* self, uint32_t i);                        \
    static inline void __Alsf(T,rm)(AList(T)* self, uint32_t i);                            \
    static inline void __Alsf(T,rm_p)(AList(T)* self, T* p);                                \
    static inline void __Alsf(T,ins)(AList(T)*self, uint32_t i, const T obj);               \
    static inline void __Alsf(T,take)(AList(T)* self,uint32_t i, T* tar);                   \
    static inline void __Alsf(T,take_p)(AList(T)* self, T* p, T* tar);                      \
    static inline void __Alsf(T,pushBack)(AList(T)* self, const T obj);                     \
    static inline void __Alsf(T,pushFront)(AList(T)* self, const T obj);                    \
    static inline void __Alsf(T,popBack)(AList(T)* self, T* tar);                           \
    static inline void __Alsf(T,popFront)(AList(T)* self, T* tar);                          \
    static inline uint32_t __Alsf(T,getNumber)(const AList(T)* self);                       \
    static inline bool __Alsf(T,empty)(const AList(T)* self);                               \
    static inline AIter(AList(T)) __Alsf(T, iter_head)(const AList(T)* self);               \
    static inline AIter(AList(T)) __Alsf(T, iter_tail)(const AList(T)* self);               \
    static inline void __Alsf(T, iter_next)(AIter(AList(T))* it);                           \
    static inline void __Alsf(T, iter_prev)(AIter(AList(T))* it);                           \
    static inline void __A_OBJ_DEST_FUNC_SELF(AList(T))(AList(T)*);                         \
    static inline void __A_OBJ_INIT_FUNC_SELF(AList(T))(AList(T)*);                         \
    static inline void __A_OBJ_COPY_FUNC_SELF(AList(T))(AList(T)*, const AList(T)*);        \
    static inline int __A_OBJ_CMPD_FUNC_SELF(AList(T))(const AList(T)*,const AList(T)*);    \
                                                                                            \
    static const A_FUNC(AList(T)) A_FUNC_TAB(AList(T)) = {                                  \
        true,                                                                               \
        (void*)__A_OBJ_DEST_FUNC_SELF(AList(T)),                                            \
        __Alsf(T,at),                                                                       \
        __Alsf(T,rm),                                                                       \
        __Alsf(T,rm_p),                                                                     \
        __Alsf(T,ins),                                                                      \
        __Alsf(T,take),                                                                     \
        __Alsf(T,take_p),                                                                   \
        __Alsf(T,pushBack),                                                                 \
        __Alsf(T,pushFront),                                                                \
        __Alsf(T,popBack),                                                                  \
        __Alsf(T,popFront),                                                                 \
        __Alsf(T,getNumber),                                                                \
        __Alsf(T,empty),                                                                    \
        __Alsf(T,iter_head),                                                                \
        __Alsf(T,iter_tail),                                                                \
        __Alsf(T,iter_next),                                                                \
        __Alsf(T,iter_prev),                                                                \
    };                                                                                      \

#define AList_Generate(T)                                                                   \
    static inline T* __Alsf(T,at)(const AList(T)* self, uint32_t index){                    \
        return __Alist_at(&self->list, index);                                              \
    }                                                                                       \
    static inline void __Alsf(T,rm)(AList(T)* self, uint32_t index){                        \
        __Alist_rm(&self->list, index);                                                     \
    }                                                                                       \
    static inline void __Alsf(T,rm_p)(AList(T)* self, T* p){                                \
        __Alist_rm_node(&self->list, __Alist_getNode(p), true);                             \
    }                                                                                       \
    static inline void __Alsf(T,ins)(AList(T)* self,uint32_t index,const T obj){            \
        __Alist_ins(&self->list, index, &obj);                                              \
    }                                                                                       \
    static inline void __Alsf(T,take)(AList(T)* self,uint32_t index, T* tar){               \
        __Alist_take(&self->list, index, tar);                                              \
    }                                                                                       \
    static inline void __Alsf(T,take_p)(AList(T)* self, T* p, T* tar){                      \
        __Alist_take_node(&self->list, __Alist_getNode(p), tar);                            \
    }                                                                                       \
    static inline void __Alsf(T,pushBack)(AList(T)* self, const T obj){                     \
        __Alist_pushBack(&self->list, &obj);                                                \
    }                                                                                       \
    static inline void __Alsf(T,pushFront)(AList(T)* self, const T obj){                    \
        __Alist_pushFront(&self->list, &obj);                                               \
    }                                                                                       \
    static inline void __Alsf(T,popBack)(AList(T)* self, T* tar){                           \
        __Alist_popBack(&self->list, tar);                                                  \
    }                                                                                       \
    static inline void __Alsf(T,popFront)(AList(T)* self, T* tar){                          \
        __Alist_popFront(&self->list, tar);                                                 \
    }                                                                                       \
    static inline uint32_t __Alsf(T,getNumber)(const AList(T)* self){                       \
        return self->list.num;                                                              \
    }                                                                                       \
    static inline bool __Alsf(T,empty)(const AList(T)* self){                               \
        return self->list.num == 0;                                                         \
    }                                                                                       \
    static inline AIter(AList(T)) __Alsf(T,iter_head)(const AList(T)* self){                \
        AIter(AList(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };                 \
        if(self->list.num != 0){                                                            \
            it.p = __Alist_getObj(self->list.head);                                         \
            it.i = 0;                                                                       \
        }                                                                                   \
        return it;                                                                          \
    }                                                                                       \
    static inline AIter(AList(T)) __Alsf(T,iter_tail)(const AList(T)* self){                \
        AIter(AList(T)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };                 \
        if(self->list.num != 0){                                                            \
            it.p = __Alist_getObj(self->list.tail);                                         \
            it.i = self->list.num - 1;                                                      \
        }                                                                                   \
        return it;                                                                          \
    }                                                                                       \
    static inline void __Alsf(T,iter_next)(AIter(AList(T))* it){                            \
        __AlsNode* node = __Alist_getNode(it->p);                                           \
        it->p = __Alist_getObj(node->next);                                                 \
        it->i++;                                                                            \
    }                                                                                       \
    static inline void __Alsf(T,iter_prev)(AIter(AList(T))* it){                            \
        __AlsNode* node = __Alist_getNode(it->p);                                           \
        it->p = __Alist_getObj(node->prev);                                                 \
        it->i--;                                                                            \
    }                                                                                       \
                                                                                            \
    __noused static inline void A_OBJ_DEST(AList(T))(AList(T)* self){                       \
        __Alist_dest(&self->list);                                                          \
    }                                                                                       \
    __noused static inline void A_OBJ_INIT(AList(T))(AList(T)* self){                       \
        self->list.size = sizeof(T);                                                        \
        self->list.copy = (void*)__A_OBJ_COPY_FUNC_SELF(T);                                 \
        self->list.dest = (void*)__A_OBJ_DEST_FUNC_SELF(T);                                 \
        self->list.cmpd = (void*)__A_OBJ_CMPD_FUNC_SELF(T);                                 \
        self->f = &A_FUNC_TAB(AList(T));                                                    \
    }                                                                                       \
    __noused static inline void A_OBJ_COPY(AList(T))                                        \
    (AList(T)* self, const AList(T)* that){                                                 \
        self->f = that->f;                                                                  \
        __Alist_copy(&self->list, &that->list);                                             \
    }                                                                                       \
    __noused static inline int  A_OBJ_CMPD(AList(T))                                        \
    (const AList(T)* self, const AList(T)* that){                                           \
        return __Alist_cmpd(&self->list, &that->list);                                      \
    }                                                                                       \


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__alist_h__*/

