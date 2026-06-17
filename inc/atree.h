/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __atree_h__
#define __atree_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aiter.h"

typedef struct __AtrNode __AtrNode;
struct __AtrNode{
    __AtrNode *par;
    __AtrNode *left, *right;
    size_t color;
    char data[0];//kV
};

__AtrNode* __AtrNode_min(__AtrNode* root);
__AtrNode* __AtrNode_max(__AtrNode* root);
__AtrNode* __AtrNode_prev(__AtrNode* node);
__AtrNode* __AtrNode_next(__AtrNode* node);

typedef struct{
    __AtrNode* root;
    uint32_t num;
    uint32_t size;
    void(*dest)(void*);
    void(*copy)(void*,const void*);
    int (*cmpd)(const void*,const void*);
    int (*cmpd_k)(const void*,const void*);
}__Atree;

void __Atree_dest(__Atree* tree);
void __Atree_copy(__Atree* tree, const __Atree* that_tree);
int __Atree_cmpd(const __Atree* tree, const __Atree* that_tree);

__AtrNode* __Atree_new_node(__Atree* tree, const void* data);

__AtrNode*__Atree_at(const __Atree* tree, const void* data);
void      __Atree_rm(__Atree* tree, __AtrNode* node);
int       __Atree_ins(__Atree* tree, __AtrNode* node);
void      __Atree_take(__Atree* tree, void* data);


#define ATree(TK,TV) __A_3Splice(__A_Generic_Tree_$__, TK, TV)
#define __Atrf(TK,TV, name) __A_Splice(__A_Splice(ATree(TK,TV), __func_$__), name)
#define __Atr_data(TK,TV) __A_Splice(ATree(TK,TV), _$__data__)

#define ATree_Define(TK,TV)                                                                     \
    typedef struct ATree(TK,TV) ATree(TK,TV);                                                   \
    typedef struct A_FUNC(ATree(TK,TV)) A_FUNC(ATree(TK,TV));                                   \
    struct ATree(TK,TV){                                                                        \
        const A_FUNC(ATree(TK,TV))* f;                                                          \
        __Atree tree;                                                                           \
        TV* type[0];                                                                            \
    };                                                                                          \
                                                                                                \
    AIter_Define(ATree(TK,TV));                                                                 \
                                                                                                \
    struct A_FUNC(ATree(TK,TV)){                                                                \
        bool    flag;                                                                           \
        void    (*dest)(void*);                                                                 \
        TV*     (*const at)       (const ATree(TK,TV)* self, const TK k);                       \
        void    (*const rm)       (ATree(TK,TV)* self, const TK k);                             \
        void    (*const ins)      (ATree(TK,TV)* self, const TK k, const TV s);                 \
        void    (*const take)     (ATree(TK,TV)* self, const TK k, TV* target);                 \
        uint32_t(*const getNumber)(const ATree(TK,TV)* self);                                   \
        bool    (*const empty)    (const ATree(TK,TV)* self);                                   \
        AIter(ATree(TK,TV)) (*const head)(const ATree(TK,TV)* self);                            \
        AIter(ATree(TK,TV)) (*const tail)(const ATree(TK,TV)* self);                            \
        void    (*const next)(AIter(ATree(TK,TV))* it);                                         \
        void    (*const prev)(AIter(ATree(TK,TV))* it);                                         \
        TK      (*const getk)(AIter(ATree(TK,TV)) it);                                          \
    };                                                                                          \
                                                                                                \
    static inline TV*      __Atrf(TK,TV,at)       (const ATree(TK,TV)* self, const TK k);       \
    static inline void     __Atrf(TK,TV,rm)       (ATree(TK,TV)* self, const TK k);             \
    static inline void     __Atrf(TK,TV,ins)      (ATree(TK,TV)* self, const TK k, const TV s); \
    static inline void     __Atrf(TK,TV,take)     (ATree(TK,TV)* self, const TK k, TV* target); \
    static inline uint32_t __Atrf(TK,TV,getNumber)(const ATree(TK,TV)* self);                   \
    static inline bool     __Atrf(TK,TV,empty)    (const ATree(TK,TV)* self);                   \
    static inline AIter(ATree(TK,TV)) __Atrf(TK,TV,iter_head)(const ATree(TK,TV)* self);        \
    static inline AIter(ATree(TK,TV)) __Atrf(TK,TV,iter_tail)(const ATree(TK,TV)* self);        \
    static inline void     __Atrf(TK,TV,iter_next)(AIter(ATree(TK,TV))* it);                    \
    static inline void     __Atrf(TK,TV,iter_prev)(AIter(ATree(TK,TV))* it);                    \
    static inline TK       __Atrf(TK,TV,iter_getk)(AIter(ATree(TK,TV)) it);                     \
    static inline void __A_OBJ_DEST_FUNC_SELF(ATree(TK,TV))(ATree(TK,TV)*);                     \
    static inline bool __A_OBJ_INIT_FUNC_SELF(ATree(TK,TV))(ATree(TK,TV)*);                     \
    static inline bool __A_OBJ_COPY_FUNC_SELF(ATree(TK,TV))(ATree(TK,TV)*, const ATree(TK,TV)*);     \
    static inline int __A_OBJ_CMPD_FUNC_SELF(ATree(TK,TV))(const ATree(TK,TV)*,const ATree(TK,TV)*); \
                                                                                                \
    static const A_FUNC(ATree(TK,TV)) A_FUNC_TAB(ATree(TK,TV)) = {                              \
        true,                                                                                   \
        (void*)__A_OBJ_DEST_FUNC_SELF(ATree(TK,TV)),                                            \
        __Atrf(TK,TV,at),                                                                       \
        __Atrf(TK,TV,rm),                                                                       \
        __Atrf(TK,TV,ins),                                                                      \
        __Atrf(TK,TV,take),                                                                     \
        __Atrf(TK,TV,getNumber),                                                                \
        __Atrf(TK,TV,empty),                                                                    \
        __Atrf(TK,TV,iter_head),                                                                \
        __Atrf(TK,TV,iter_tail),                                                                \
        __Atrf(TK,TV,iter_next),                                                                \
        __Atrf(TK,TV,iter_prev),                                                                \
        __Atrf(TK,TV,iter_getk),                                                                \
    };                                                                                          \

#define ATree_Generate(TK,TV)                                                                   \
    typedef struct { TK k; TV v; } __Atr_data(TK,TV);                                           \
    static inline void __Atrf(TK,TV,data_dest)(__Atr_data(TK,TV)* data){                        \
        A_DEST(TK, data->k); A_DEST(TV, data->v);                                               \
    }                                                                                           \
    static inline void __Atrf(TK,TV,data_copy)(__Atr_data(TK,TV)* data,                         \
            const __Atr_data(TK,TV)* that_data){                                                \
        aTry(data->k = A_COPY(TK, that_data->k);)aExc{                                          \
            return;                                                                             \
        }                                                                                       \
        aTry(data->v=A_COPY(TV,that_data->v);)aExc{                                             \
            A_DEST(TK, data->k); return;                                                        \
        }                                                                                       \
    }                                                                                           \
    static inline int  __Atrf(TK,TV,data_cmpd)(const __Atr_data(TK,TV)* data,                   \
            const __Atr_data(TK,TV)* that_data){                                                \
        int ret = A_CMPD(TK, data->k, that_data->k);                                            \
        if(ret == 0) ret = A_CMPD(TV, data->v, that_data->v);                                   \
        return ret;                                                                             \
    }                                                                                           \
    /******************************************************************************************/\
    static inline TV* __Atrf(TK,TV,at)(const ATree(TK,TV)* self, const TK k){                   \
        __AtrNode* node = __Atree_at(&self->tree, (const void*)&k);                             \
        if(__a_unlikely(node == nullptr)){ return nullptr; }                                    \
        return &(((__Atr_data(TK,TV)*)&(node->data[0]))->v);                                    \
    }                                                                                           \
    static inline void __Atrf(TK,TV,rm)(ATree(TK,TV)* self, const TK k){                        \
        __AtrNode* node = __Atree_at(&self->tree, (const void*)&k);                             \
        __Atree_rm(&self->tree, node);                                                          \
    }                                                                                           \
    static inline void __Atrf(TK,TV,ins)(ATree(TK,TV)* self, const TK k, const TV v){           \
        aErrClean();                                                                            \
        __AtrNode* node = __Atree_new_node(&self->tree, &(__Atr_data(TK,TV)){.k = k,.v = v});   \
        if(__a_unlikely(node == nullptr)){ aErrSet(AERR_alloc_failed); return; };               \
        int ret = __Atree_ins(&self->tree, node); if(__a_unlikely(ret != 0)) aErrSet(ret);      \
    }                                                                                           \
    static inline void __Atrf(TK,TV,take)(ATree(TK,TV)* self, const TK k, TV* tar){             \
        if(tar != nullptr) memset(tar, 0, sizeof(TV));                                          \
        aTry(__Atr_data(TK,TV) data = { .k = k }; __Atree_take(&self->tree, (void*)&data);)aExc{\
            return;                                                                             \
        }                                                                                       \
        A_DEST(TK, data.k); if(tar != nullptr) *tar = data.v; else A_DEST(TV, data.v);          \
    }                                                                                           \
    static inline uint32_t __Atrf(TK,TV,getNumber)(const ATree(TK,TV)* self){                   \
        return self->tree.num;                                                                  \
    }                                                                                           \
    static inline bool __Atrf(TK,TV,empty)(const ATree(TK,TV)* self){                           \
        return self->tree.root == nullptr;                                                      \
    }                                                                                           \
    static inline AIter(ATree(TK,TV)) __Atrf(TK,TV,iter_head)(const ATree(TK,TV)* self){        \
        AIter(ATree(TK,TV)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };                 \
        __AtrNode* node = __AtrNode_min(self->tree.root);                                       \
        __Atr_data(TK,TV)* data = node != nullptr ? (void*)&(node->data[0]) : nullptr;          \
        it.p = data != nullptr ? &(data->v) : nullptr;                                          \
        it.i = it.p != nullptr ? 0 : 0;                                                         \
        return it;                                                                              \
    }                                                                                           \
    static inline AIter(ATree(TK,TV)) __Atrf(TK,TV,iter_tail)(const ATree(TK,TV)* self){        \
        AIter(ATree(TK,TV)) it = { .con = self, .p = nullptr, .i = 0, .r = 0 };                 \
        __AtrNode* node = __AtrNode_max(self->tree.root);                                       \
        __Atr_data(TK,TV)* data = node != nullptr ? (void*)&(node->data[0]) : nullptr;          \
        it.p = data != nullptr ? &(data->v) : nullptr;                                          \
        it.i = it.p != nullptr ? self->tree.num - 1 : 0;                                        \
        return it;                                                                              \
    }                                                                                           \
    static inline void __Atrf(TK,TV,iter_next)(AIter(ATree(TK,TV))* it){                        \
        if(__a_unlikely(it->p == nullptr)) return;                                              \
        __Atr_data(TK,TV)* data = container_of(it->p,__Atr_data(TK,TV),v);                      \
        __AtrNode* node = container_of((void*)data, __AtrNode, data);                           \
        node = __AtrNode_next(node); data = node != nullptr ? (void*)&(node->data[0]) : nullptr;\
        it->p = data != nullptr ? &(data->v) : nullptr;                                         \
        it->i++;                                                                                \
    }                                                                                           \
    static inline void __Atrf(TK,TV,iter_prev)(AIter(ATree(TK,TV))* it){                        \
        if(__a_unlikely(it->p == nullptr)) return;                                              \
        __Atr_data(TK,TV)* data = container_of(it->p,__Atr_data(TK,TV),v);                      \
        __AtrNode* node = container_of((void*)data, __AtrNode, data);                           \
        node = __AtrNode_prev(node); data = node != nullptr ? (void*)&(node->data[0]) : nullptr;\
        it->p = data != nullptr ? &(data->v) : nullptr;                                         \
        it->i--;                                                                                \
    }                                                                                           \
    static inline TK __Atrf(TK,TV,iter_getk)(AIter(ATree(TK,TV)) it){                           \
        TK k; memset(&k, 0, sizeof(TK));                                                        \
        if(__a_unlikely(it.p == nullptr)){ aErrSet(AERR_overstep); return k; }                  \
        return container_of(it.p,__Atr_data(TK,TV),v)->k;                                       \
    }                                                                                           \
                                                                                                \
    __noused static inline void A_OBJ_INIT(ATree(TK,TV))(ATree(TK,TV)* self){                   \
        self->f = &A_FUNC_TAB(ATree(TK,TV));                                                    \
        self->tree.size = sizeof(__Atr_data(TK,TV));                                            \
        self->tree.copy = (void*)__Atrf(TK,TV,data_copy);                                       \
        self->tree.dest = (void*)__Atrf(TK,TV,data_dest);                                       \
        self->tree.cmpd = (void*)__Atrf(TK,TV,data_cmpd);                                       \
        self->tree.cmpd_k = (void*)__A_OBJ_CMPD_FUNC_SELF(TK);                                  \
    }                                                                                           \
    __noused static inline void A_OBJ_DEST(ATree(TK,TV))(ATree(TK,TV)* self){                   \
        __Atree_dest(&self->tree);                                                              \
    }                                                                                           \
    __noused static inline void A_OBJ_COPY(ATree(TK,TV))                                        \
    (ATree(TK,TV)* self, const ATree(TK,TV)* that){                                             \
        self->f = that->f;                                                                      \
        __Atree_copy(&self->tree, &that->tree);                                                 \
    }                                                                                           \
    __noused static inline int  A_OBJ_CMPD(ATree(TK,TV))                                        \
    (const ATree(TK,TV)*self,const ATree(TK,TV)*that){                                          \
        return __Atree_cmpd(&self->tree, &that->tree);                                          \
    }                                                                                           \



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__atree_h__*/

