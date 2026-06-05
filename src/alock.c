/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <alock.h>

/* 解锁 */
void AMtx_unlock(AMtx* self){
    if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }
    if(mtx_unlock(&((ALock*)self)->mtx) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* 加锁 */
void AMtx_uplock(AMtx* self){
    if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }
    if(mtx_lock(&((ALock*)self)->mtx) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* AAutoKey */
AAutoKey AMtx_lock(AMtx* self){
    aExcClean(); AAutoKey key = {}; AMtx_uplock(self);
    if(!aExcOccur()){
        key.lock = (ALock*)self, key.unlock = (void*)AMtx_unlock;
    }
    return key;
}



/* 解锁 */
void ARecursion_unlock(ARecursion* self){
    if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }
    if(mtx_unlock(&((ALock*)self)->mtx) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* 加锁 */
void ARecursion_uplock(ARecursion* self){
    if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }
    if(mtx_lock(&((ALock*)self)->mtx) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* AAutoKey */
AAutoKey ARecursion_lock(ARecursion* self){
    aExcClean(); AAutoKey key = {}; ARecursion_uplock(self);
    if(!aExcOccur()){
        key.lock = (ALock*)self, key.unlock = (void*)ARecursion_unlock;
    }
    return key;
}



/* read解锁 */
void AMtxRW_unlock_read(AMtxRW* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto lock = &((ALock*)self)->mtx;
    if(mtx_lock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    if(__a_unlikely(self->reader_num == 0)){
        mtx_unlock(lock);
        aExcSet(AEXC_system_error);
        return;
    }

    self->reader_num--;
    if(self->reader_num == 0 && self->writer_wait != 0){
        if(cnd_signal(&self->write_cond) != thrd_success){
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }

    if(mtx_unlock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* read加锁 */
void AMtxRW_uplock_read(AMtxRW* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto lock = &((ALock*)self)->mtx;
    if(mtx_lock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    while(self->writer_hold || self->writer_wait != 0){
        if(cnd_wait(&self->read_cond, lock) != thrd_success){
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }

    self->reader_num++;

    if(mtx_unlock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* AAutoKey */
AAutoKey AMtxRW_rlock(AMtxRW* self){
    aExcClean(); AAutoKey key = {}; AMtxRW_uplock_read(self);
    if(!aExcOccur()){
        key.lock = (ALock*)self, key.unlock = (void*)AMtxRW_unlock_read;
    }
    return key;
}

/* write解锁 */
void AMtxRW_unlock_write(AMtxRW* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto lock = &((ALock*)self)->mtx;
    if(mtx_lock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    if(__a_unlikely(!self->writer_hold)){
        mtx_unlock(lock);
        aExcSet(AEXC_system_error);
        return;
    }

    self->writer_hold = false;
    if(self->writer_wait != 0){
        if(cnd_signal(&self->write_cond) != thrd_success){
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }else{
        if(cnd_broadcast(&self->read_cond) != thrd_success){
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }

    if(mtx_unlock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* write加锁 */
void AMtxRW_uplock_write(AMtxRW* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto lock = &((ALock*)self)->mtx;
    if(mtx_lock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    self->writer_wait++;
    while(self->writer_hold || self->reader_num != 0){
        if(cnd_wait(&self->write_cond, lock) != thrd_success){
            self->writer_wait--;
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }
    self->writer_wait--;
    self->writer_hold = true;

    if(mtx_unlock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* AAutoKey */
AAutoKey AMtxRW_wlock(AMtxRW* self){
    aExcClean(); AAutoKey key = {}; AMtxRW_uplock_write(self);
    if(!aExcOccur()){
        key.lock = (ALock*)self, key.unlock = (void*)AMtxRW_unlock_write;
    }
    return key;
}



/* 解锁 */
void AMtxCnd_unlock(AMtxCnd* self){
    if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }
    if(mtx_unlock(&((ALock*)self)->mtx) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* 加锁 */
void AMtxCnd_uplock(AMtxCnd* self){
    if(__a_unlikely(self == nullptr)){ aExcSet(AEXC_nullptr); return; }
    if(mtx_lock(&((ALock*)self)->mtx) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* AAutoKey */
AAutoKey AMtxCnd_lock(AMtxCnd* self){
    aExcClean(); AAutoKey key = {}; AMtxCnd_uplock(self);
    if(!aExcOccur()){
        key.lock = (ALock*)self, key.unlock = (void*)AMtxCnd_unlock;
    }
    return key;
}



void ASemaphore_setMax(ASemaphore* self, uint32_t max){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto lock = &((ALock*)self)->mtx;
    if(mtx_lock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    uint32_t last_max = self->max;
    self->max = max;
    if(max > last_max && self->count < self->max){
        if(cnd_broadcast(&((AMtxCnd*)self)->cnd) != thrd_success){
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }

    if(mtx_unlock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}

/* 解锁 */
void ASemaphore_unlock(ASemaphore* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto lock = &((ALock*)self)->mtx;
    if(mtx_lock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    if(__a_unlikely(self->count == 0)){
        mtx_unlock(lock);
        aExcSet(AEXC_system_error);
        return;
    }

    self->count--;
    if(self->count < self->max){
        if(cnd_signal(&((AMtxCnd*)self)->cnd) != thrd_success){
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }

    if(mtx_unlock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* 加锁 */
void ASemaphore_uplock(ASemaphore* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto lock = &((ALock*)self)->mtx;
    if(mtx_lock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    while(self->count >= self->max){
        if(cnd_wait(&((AMtxCnd*)self)->cnd, lock) != thrd_success){
            mtx_unlock(lock);
            aExcSet(AEXC_system_error);
            return;
        }
    }
    self->count++;

    if(mtx_unlock(lock) != thrd_success) {
        aExcSet(AEXC_system_error);
    }
}
/* AAutoKey */
AAutoKey ASemaphore_lock(ASemaphore* self){
    aExcClean(); AAutoKey key = {}; ASemaphore_uplock(self);
    if(!aExcOccur()){
        key.lock = (ALock*)self, key.unlock = (void*)ASemaphore_unlock;
    }
    return key;
}


