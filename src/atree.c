/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <atree.h>

/* ---------- 红黑树辅助函数 ---------- */
static inline void __AtrNode_set_black(__AtrNode* node){
    if(__a_likely(node != nullptr)) node->color = 1;
}
static inline void __AtrNode_set_red(__AtrNode* node){
    if(__a_likely(node != nullptr)) node->color = 0;
}
static inline bool __AtrNode_is_black(const __AtrNode* node){
    return node == nullptr || node->color == 1 ? true : false;
}
static inline bool __AtrNode_is_red(const __AtrNode* node){
    return !__AtrNode_is_black(node);
}
static inline void __AtrNode_swap(__AtrNode* x, __AtrNode* y, uint32_t size){
    char buf[size];
    memset(buf, 0, size);

    memcpy(buf, x->data, size);
    memcpy(x->data, y->data, size);
    memcpy(y->data, buf, size);
}


/* ---------- 辅助栈 ---------- */
typedef struct{
    uint32_t cap;
    uint32_t num;
    __AtrNode** p;
} __Astc;

static inline void __Astc_init(__Astc* stc) {
    memset(stc, 0, sizeof(*stc));
}

static inline void __Astc_dest(__Astc* stc) {
    alib_free(stc->p);
}

static inline int __Astc_add_cap(__Astc* stc) {
    uint32_t cap = stc->cap != 0 ? stc->cap * 2 : 8;
    __AtrNode** p = alib_realloc(stc->p, sizeof(__AtrNode*) * cap);
    if (__a_unlikely(p == nullptr)) {
        return AEXC_alloc_failed;
    }
    stc->p = p;
    stc->cap = cap;
    return 0;
}

static inline int __Astc_push(__Astc* stc, __AtrNode* node) {
    if (__a_unlikely(stc->cap <= stc->num)) {
        int ret = __Astc_add_cap(stc);
        if(ret != 0) return ret;
    }
    stc->p[stc->num++] = node;
    return 0;
}

static inline __AtrNode* __Astc_pop(__Astc* stc) {
    if (__a_unlikely(stc->num == 0)) {
        return nullptr;
    }
    return stc->p[--stc->num];
}

static inline void* __AtrNode_get_data(__AtrNode* node){
    return node != nullptr ? node->data : nullptr;
}

 __AtrNode* __AtrNode_min(__AtrNode* root){
    __AtrNode* node = root;
    while(node != nullptr && node->left != nullptr){
        node = node->left;
    }

    return node;
}

__AtrNode* __AtrNode_max(__AtrNode* root){
    __AtrNode* node = root;
    while(node != nullptr && node->right != nullptr){
        node = node->right;
    }

    return node;
}

__AtrNode* __AtrNode_prev(__AtrNode* node){
    if(node->left != nullptr){
        return __AtrNode_max(node->left);
    }else{
        __AtrNode* x = node;
        while(x != nullptr && x->par != nullptr){
            if(x == x->par->left){
                x = x->par;
            }else{
                return x->par;
            }
        }
    }
    return nullptr;
}

__AtrNode* __AtrNode_next(__AtrNode* node){
    if(node->right != nullptr){
        return __AtrNode_min(node->right);
    }else{
        __AtrNode* x = node;
        while(x != nullptr && x->par != nullptr){
            if(x == x->par->right){
                x = x->par;
            }else{
                return x->par;
            }
        }
    }
    return nullptr;
}

__AtrNode* __Atree_new_node(__Atree* tree, const void* data){
    __AtrNode* node = alib_alloc(sizeof(__AtrNode) + tree->size);
    if(__a_unlikely(node == nullptr)) return nullptr;
    memset(node, 0, sizeof(__AtrNode));

    __AtrNode_set_red(node);
    tree->copy(__AtrNode_get_data(node), data);
    if(aExcOccur()){
        alib_free(node); node = nullptr;
    }
    return node;
}

/* -------------------------------------- */
/* 提升当前节点，相当于父节点左旋或右旋，此处统一处理 */
static inline void  __Atree_up_node(__Atree* tree, __AtrNode* n){
    if(__a_unlikely(n == nullptr) || n->par == nullptr){
        return;
    }

    if(n == n->par->right){
        __AtrNode* x = n->par;
        __AtrNode* p = x->par;
        __AtrNode* l = n->left;

        n->par = p, n->left = x;
        x->par = n, x->right = l;

        if(p != nullptr){
            if(p->left == x){
                p->left = n;
            }else{
                p->right = n;
            }
        }

        if(l != nullptr){
            l->par = x;
        }
    }else{
        __AtrNode* x = n->par;
        __AtrNode* p = x->par;
        __AtrNode* r = n->right;

        n->par = p, n->right = x;
        x->par = n, x->left = r;

        if(p != nullptr){
            if(p->left == x){
                p->left = n;
            }else{
                p->right = n;
            }
        }

        if(r != nullptr){
            r->par = x;
        }
    }

    if(n->par == nullptr){
        tree->root = n;
    }
}
static inline __AtrNode* __Atree_find(const __Atree* tree, const void* data){
    int  ret = 0;
    __AtrNode* node = tree->root;
    while(node != nullptr){
        ret = tree->cmpd_k(data, __AtrNode_get_data(node));
        if(ret == 0){
            break;
        }else if(ret > 0){
            node = node->right;
        }else{
            node = node->left;
        }
    }
    return node;
}
static inline void __Atree_freeNode(__Atree* tree, __AtrNode* node, bool de){
    if(de) tree->dest(__AtrNode_get_data(node));
    alib_free(node);
}
static inline void __Atree_delNode(__Atree* tree, __AtrNode* node, bool de){
    if(__a_unlikely(node == nullptr)){
        return;
    }

    if(node->par != nullptr){
        if(node == node->par->left){
            node->par->left = nullptr;
        }else{
            node->par->right = nullptr;
        }
    }else{
        tree->root = nullptr;
    }

    __Atree_freeNode(tree, node, de);
}
static inline void __Atree_delNode_rec(__Atree* tree, __AtrNode* node){
    if(__a_unlikely(node == nullptr)){
        return;
    }

    if(node->par != nullptr){
        if(node == node->par->left){
            node->par->left = nullptr;
        }else{
            node->par->right = nullptr;
        }
    }else{
        tree->root = nullptr;
    }

    if(node->left != nullptr){
        __Atree_delNode_rec(tree, node->left);
        node->left = nullptr;
    }

    if(node->right != nullptr){
        __Atree_delNode_rec(tree, node->right);
        node->right = nullptr;
    }

    tree->dest(__AtrNode_get_data(node));
    alib_free(node);
}

/* -------------------------------------- */
void __Atree_dest(__Atree* tree){
    if(__a_unlikely(tree->root == nullptr)) return;

    __Astc stc;
    __Astc_init(&stc);
    int ret = __Astc_push(&stc, tree->root);
    if(ret != 0){
        __Astc_dest(&stc);
        return;
    }

    while(stc.num != 0){
        __AtrNode* node = __Astc_pop(&stc);

        if(node->right != nullptr){
            node->right->par = nullptr;
            ret = __Astc_push(&stc, node->right);
            if(ret != 0){
                __Atree_delNode_rec(tree, node);
                break;
            }
        }
        if(node->left != nullptr){
            node->left->par = nullptr;
            ret = __Astc_push(&stc, node->left);
            if(ret != 0){
                __Atree_delNode_rec(tree, node);
                break;
            }
        }

        __Atree_delNode(tree, node ,true);
    }

    //若push发生异常
    for(uint32_t i = 0; i < stc.num; i++){
        __AtrNode* node = __Astc_pop(&stc);
        __Atree_delNode_rec(tree, node);
    }

    __Astc_dest(&stc);
}
void __Atree_copy(__Atree* tree, const __Atree* that_tree){
    *tree = *that_tree;
    tree->num = 0;
    tree->root = nullptr;

     if(__a_unlikely(that_tree->root == nullptr)){
         return;
     }

    __Astc stc;
    __Astc_init(&stc);
    int ret = __Astc_push(&stc, that_tree->root);

    while(stc.num != 0 && ret == 0){
        __AtrNode* node = __Astc_pop(&stc);
        if(__a_unlikely(node == nullptr)) break;

        if(node->right != nullptr){
            ret = __Astc_push(&stc, node->right);
        }
        if(node->left != nullptr){
            ret = __Astc_push(&stc, node->left);
        }

        __AtrNode* new_node = __Atree_new_node(tree, __AtrNode_get_data(node));
        if(__a_unlikely(new_node == nullptr)){
            ret = AEXC_alloc_failed;
        }else{
            ret = __Atree_ins(tree, new_node);
        }
    }

    __Astc_dest(&stc);

    if(__a_unlikely(ret != 0)){
        aExcSet(AEXC_init_failed);
    }
}

int __Atree_cmpd(const __Atree* tree, const __Atree* that_tree){
    int ret = 0;

    if(tree->root == nullptr && that_tree->root == nullptr){
        return 0;
    }else if(tree->root == nullptr){
        return -1;
    }else if(that_tree->root == nullptr){
        return 1;
    }

    __AtrNode* node_self = __AtrNode_min(tree->root);
    __AtrNode* node_that = __AtrNode_min(that_tree->root);
    while(node_self != nullptr && node_that != nullptr){
        ret = tree->cmpd(__AtrNode_get_data(node_self), __AtrNode_get_data(node_that));
        if(ret != 0){
            return ret;
        }else{
            node_self = __AtrNode_next(node_self);
            node_that = __AtrNode_next(node_that);
        }
    }

    if(node_self == nullptr || node_that == nullptr){
        if(node_self == nullptr && node_that == nullptr){
            ret = 0;
        }else if(node_self == nullptr){
            ret = -1;
        }else{
            ret = 1;
        }
    }

    return ret;
}


/* -------------------------------------- */
__AtrNode* __Atree_at(const __Atree* tree, const void* data){
    if(__a_unlikely(data == nullptr)) return nullptr;
    return __Atree_find(tree, data);
}

static inline void __Atree_remove(__Atree* tree, __AtrNode* node, bool de);
void __Atree_rm(__Atree* tree, __AtrNode* node){
    if(__a_unlikely(node == nullptr)){
        aExcSet(AEXC_overstep);
        return;
    }

    __Atree_remove(tree, node, true);
    tree->num--;
}
static inline void __Atree_tk(__Atree* tree, __AtrNode* node){
    if(__a_unlikely(node == nullptr)){
        aExcSet(AEXC_overstep);
        return;
    }

    __Atree_remove(tree, node, false);
    tree->num--;
}


static inline int __Atree_install(__Atree* tree, __AtrNode* node);
int __Atree_ins(__Atree* tree, __AtrNode* node){
    int ret = 0;

    if(tree->root == nullptr){
        tree->num++;
        tree->root = node;
        __AtrNode_set_black(tree->root);
    }else{
        ret = __Atree_install(tree, node);
    }

    if(__a_unlikely(ret != 0)){
        aExcSet(AEXC_alloc_failed);
    }

    return ret;
}

static inline void __Atree_tk(__Atree* tree, __AtrNode* node);
void __Atree_take(__Atree* tree, void* data){
    __AtrNode* node = __Atree_find(tree, data);
    if(__a_unlikely(node == nullptr)){
        aExcSet(AEXC_overstep);
        return;
    }

    memcpy(data, __AtrNode_get_data(node), tree->size);
    memset(__AtrNode_get_data(node), 0, tree->size);

    __Atree_tk(tree, node);
}
/* -------------------------------------- */
static inline void __Atree_install_2_red(__Atree* tree, __AtrNode* node);
static inline int __Atree_install(__Atree* tree, __AtrNode* node){
    int ret = 0;
    int cmp = 0;
    __AtrNode* x = tree->root;
    while(x != nullptr){
        cmp = tree->cmpd_k(__AtrNode_get_data(node), __AtrNode_get_data(x));
        if(cmp == 0){
            break;
        }else if(cmp > 0){
            if(x->right != nullptr){
                x = x->right;
            }else{
                break;
            }
        }else{
            if(x->left != nullptr){
                x = x->left;
            }else{
                break;
            }
        }
    }

    if(cmp == 0){
        __AtrNode_swap(x, node, tree->size);
        __Atree_freeNode(tree, node ,true);
        return 0;
    }else if(cmp > 0){
        node->par = x, x->right = node;
        if(__AtrNode_is_red(x)){
            __Atree_install_2_red(tree, node);
        }
    }else{
        node->par = x, x->left = node;
        if(__AtrNode_is_red(x)){
            __Atree_install_2_red(tree, node);
        }
    }

    __AtrNode_set_black(tree->root);
    tree->num++;

    return ret;
}

static inline void __Atree_remove_2_black(__Atree* tree, __AtrNode* node, bool de);
static inline void __Atree_remove(__Atree* tree, __AtrNode* node, bool de){
    if(__a_unlikely(tree->num == 1)){
        __Atree_delNode(tree, tree->root, de);
        tree->root = nullptr;
        return;
    }

    if(node->left != nullptr){
        __AtrNode* x = __AtrNode_max(node->left);
        __AtrNode_swap(x, node, tree->size);
        node = x;
    }else if(node->right != nullptr){
        __AtrNode* x = __AtrNode_min(node->right);
        __AtrNode_swap(x, node, tree->size);
        node = x;
    }else{
    }

    if(node->left != nullptr || node->right != nullptr){
        __AtrNode* x = node->left != nullptr ? node->left : node->right;
        __AtrNode_set_red(node);
        __AtrNode_set_black(x);
         __Atree_up_node(tree, x);
        if(x->par == nullptr){
            tree->root = x;
        }
        __Atree_delNode(tree, node, de);
    }else{
        if(__AtrNode_is_red(node)){
            __Atree_delNode(tree, node, de);
        }else{
            __Atree_remove_2_black(tree, node, de);
        }
    }

    __AtrNode_set_black(tree->root);
}

/* -------------------------------------- */
static inline __AtrNode* __AtrNode_get_par(__AtrNode* node){
    if(node != nullptr){
        return node->par;
    }
    return nullptr;
}
static inline __AtrNode* __AtrNode_get_grp(__AtrNode* node){
    return __AtrNode_get_par(__AtrNode_get_par(node));
}
static inline __AtrNode* __AtrNode_get_bro(__AtrNode* node){
    if(node != nullptr && node->par != nullptr){
        if(node == node->par->left)
            return node->par->right;
        else
            return node->par->left;
    }
    return nullptr;
}
static inline __AtrNode* __AtrNode_get_unc(__AtrNode* node){
    return __AtrNode_get_bro(__AtrNode_get_par(node));
}
static inline __AtrNode* __AtrNode_get_nep_n(__AtrNode* node){
    __AtrNode* bro = __AtrNode_get_bro(node);
    if(node != nullptr && node->par != nullptr && bro != nullptr){
        if(node == node->par->left)
            return bro->left;
        else
            return bro->right;
    }
    return nullptr;
}
static inline __AtrNode* __AtrNode_get_nep_f(__AtrNode* node){
    __AtrNode* bro = __AtrNode_get_bro(node);
    if(node != nullptr && node->par != nullptr && bro != nullptr){
        if(node == node->par->left)
            return bro->right;
        else
            return bro->left;
    }
    return nullptr;
}

/* -------------------------------------- */
static inline void __Atree_install_2_red(__Atree* tree, __AtrNode* n){
    while(1){
        if(n == nullptr || n->par == nullptr){
            break;
        }
        if(__AtrNode_is_black(n) || __AtrNode_is_black(n->par)){
            break;
        }

        __AtrNode* p = __AtrNode_get_par(n);
        __AtrNode* g = __AtrNode_get_grp(n);
        __AtrNode* u = __AtrNode_get_unc(n);

        if(u != nullptr && __AtrNode_is_red(u)){
            __AtrNode_set_red(g);
            __AtrNode_set_black(u);
            __AtrNode_set_black(p);
            n = g;
        }else{
            //LL || RR
            if((n == p->left && p == g->left) || (n == p->right && p == g->right)){
                __Atree_up_node(tree, p);
                __AtrNode_set_black(p);
                __AtrNode_set_red(g);
            }else{
                __Atree_up_node(tree, n);
                n = p;
            }
        }
    }
}

static inline void __Atree_remove_2_black(__Atree* tree, __AtrNode* n, bool de){
    __AtrNode* rm_node = n;

    while(n != tree->root && __AtrNode_is_black(n)){
        __AtrNode* p = __AtrNode_get_par(n);
        __AtrNode* b = __AtrNode_get_bro(n);

        if(__AtrNode_is_red(b)){
            __Atree_up_node(tree, b);
            __AtrNode_set_black(b);
            __AtrNode_set_red(p);

            p = __AtrNode_get_par(n);
            b = __AtrNode_get_bro(n);
        }

        __AtrNode* nn = __AtrNode_get_nep_n(n);
        __AtrNode* fn = __AtrNode_get_nep_f(n);

        if(__AtrNode_is_black(nn) && __AtrNode_is_black(fn)){
            __AtrNode_set_red(b);
            n = p;
        }else{
            if(__AtrNode_is_black(fn)){
                __Atree_up_node(tree, nn);
                __AtrNode_set_black(nn);
                __AtrNode_set_red(b);

                p = __AtrNode_get_par(n);
                b = __AtrNode_get_bro(n);
                fn = __AtrNode_get_nep_f(n);
            }

            if(__AtrNode_is_red(p)){
                __AtrNode_set_red(b);
            }else{
                __AtrNode_set_black(b);
            }
            __AtrNode_set_black(p);
            __AtrNode_set_black(fn);
            __Atree_up_node(tree, b);
            n = tree->root;
        }
    }

    __AtrNode_set_black(n);
    __Atree_delNode(tree, rm_node, de);
}


