/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __alock_h__
#define __alock_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "athrd.h"
#include "aclass.h"



/* 锁的基类 */
/* 觊觎c11 mtx_t */
AClass_Inherit(ALock);
AClass_Struct(ALock,
    mtx_t mtx;
);
AClass_Function(ALock);
AClass_Generate(ALock);
A_CLASS_REGISTER(ALock);

/* 解锁 */
static inline void ALock_unlock(__unused ALock* self){}
/* 加锁 */
static inline void ALock_uplock(__unused ALock* self){}



/* 用于自动解锁 */
/* 析构时自动解锁 */
typedef struct AAutoKey AAutoKey;
struct AAutoKey{
    ALock* lock;
    void(*unlock)(ALock*);
};
static inline void A_OBJ_DEST(AAutoKey)(AAutoKey* self){
    if(self->lock != nullptr && self->unlock != nullptr){
        self->unlock(self->lock);
        memset(self, 0, sizeof(AAutoKey));
    }
}
static inline void A_OBJ_COPY(AAutoKey)(AAutoKey* self, __unused const AAutoKey* that){
    memset(self, 0, sizeof(AAutoKey));
}
A_TYPE_REGISTER(AAutoKey);



/* 互斥锁 */
AClass_Inherit(AMtx, ALock);
AClass_Struct(AMtx);
AClass_Function(AMtx);
AClass_Generate(AMtx);
__unused static inline void A_OBJ_INIT(AMtx)(AMtx* self){
    if(mtx_init(&((ALock*)self)->mtx, mtx_plain) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
__unused static inline void A_OBJ_DEST(AMtx)(AMtx* self){
    mtx_destroy(&((ALock*)self)->mtx);
}
__unused static inline void A_OBJ_COPY(AMtx)(AMtx* self, __unused const AMtx* that){
    if(mtx_init(&((ALock*)self)->mtx, mtx_plain) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
A_CLASS_REGISTER(AMtx);



/* 递归互斥锁 */
AClass_Inherit(ARecursion, ALock);
AClass_Struct(ARecursion);
AClass_Function(ARecursion);
AClass_Generate(ARecursion);
__unused static inline void A_OBJ_INIT(ARecursion)(ARecursion* self){
    if(mtx_init(&((ALock*)self)->mtx, mtx_plain | mtx_recursive) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
__unused static inline void A_OBJ_DEST(ARecursion)(ARecursion* self){
    mtx_destroy(&((ALock*)self)->mtx);
}
__unused static inline void A_OBJ_COPY(ARecursion)(ARecursion* self, __unused const ARecursion* that){
    if(mtx_init(&((ALock*)self)->mtx, mtx_plain | mtx_recursive) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
A_CLASS_REGISTER(ARecursion);



/* 基于mtx/cnd的读写锁 */
AClass_Inherit(AMtxRW, AMtx);
AClass_Struct(AMtxRW,
    cnd_t read_cond;
    cnd_t write_cond;
    uint32_t reader_num;
    uint32_t writer_wait;
    bool writer_hold;
    bool read_cond_init;
    bool write_cond_init;
);
AClass_Function(AMtxRW);
AClass_Generate(AMtxRW);
__unused static inline void __AMtxRW_reset(AMtxRW* self){
    self->reader_num = 0;
    self->writer_wait = 0;
    self->writer_hold = false;
    self->read_cond_init = false;
    self->write_cond_init = false;
}
__unused static inline void A_OBJ_INIT(AMtxRW)(AMtxRW* self){
    __AMtxRW_reset(self);

    if(cnd_init(&self->read_cond) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }
    self->read_cond_init = true;

    if(cnd_init(&self->write_cond) != thrd_success) {
        cnd_destroy(&self->read_cond);
        self->read_cond_init = false;
        aExcSet(AEXC_system_error);
        return;
    }
    self->write_cond_init = true;
}
__unused static inline void A_OBJ_DEST(AMtxRW)(AMtxRW* self){
    if(self->write_cond_init){
        cnd_destroy(&self->write_cond);
    }
    if(self->read_cond_init){
        cnd_destroy(&self->read_cond);
    }
    __AMtxRW_reset(self);
}
__unused static inline void A_OBJ_COPY(AMtxRW)(AMtxRW* self, __unused const AMtxRW* that){
    A_OBJ_INIT(AMtxRW)(self);
}
A_CLASS_REGISTER(AMtxRW);



AAutoKey AMtx_lock(AMtx* self);
AAutoKey ARecursion_lock(ARecursion* self);
AAutoKey AMtxRW_rlock(AMtxRW* self);
AAutoKey AMtxRW_wlock(AMtxRW* self);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__alock_h__*/
