/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <asignal.h>
#include <threads.h>

static mtx_t a_ss_lock;
static bool a_ss_flag = false;
static ASignalSystem a_signal_system;

__attribute__((constructor)) static inline void a_signal_system_start(){
    aExcClean();
    if(mtx_init(&a_ss_lock, mtx_plain) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    a_signal_system = A_INIT(ASignalSystem);

    if(aExcOccur()){
        mtx_destroy(&a_ss_lock);
        a_ss_flag = false;
    }else{
        a_ss_flag = true;
    }
}
__attribute__((destructor)) static inline void a_signal_system_poweroff(){
    if(a_ss_flag){
        A_DEST(ASignalSystem, a_signal_system);
        mtx_destroy(&a_ss_lock);
        a_ss_flag = false;
    }
}



int64_t a_signal_system_alloc(){
    if(!a_ss_flag){
        aExcSet(AEXC_system_error);
        return -1;
    }

    mtx_lock(&a_ss_lock);

    aExcClean();
    auto id = a_signal_system.count;

    RAII(ALine(ASignalEnd)) group = A_INIT(ALine(ASignalEnd));
    if(aExcOccur()){
        mtx_unlock(&a_ss_lock);
        return -1;
    }

    auto tab = &a_signal_system.tab;
    tab->f->ins(tab, (uint32_t)id, group);
    if(aExcOccur()){
        mtx_unlock(&a_ss_lock);
        return -1;
    }
    a_signal_system.count++;

    mtx_unlock(&a_ss_lock);
    return id;
}



static inline void a_signal_call(const ASignal* signal, ASignalEnd* end){
    end->call(signal, end->addressee);
}

static inline void a_signal_group_call(const ASignal* signal, ALine(ASignalEnd)* group){
    forEach(it, *group){
        a_signal_call(signal, it.p);
    }
}

void a_signal_system_transmit(const ASignal* signal){
    if(!a_ss_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    if(__a_unlikely(signal == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(signal->id < 0)){
        aExcSet(AEXC_overstep);
        return;
    }

    mtx_lock(&a_ss_lock);
    aExcClean();

    auto id = signal->id;
    if(__a_unlikely(id >= a_signal_system.count)){
        aExcSet(AEXC_overstep);
        mtx_unlock(&a_ss_lock);
        return;
    }

    auto tab = &a_signal_system.tab;
    auto group = tab->f->at(tab, (uint32_t)id);
    if(__a_unlikely(group == nullptr)){
        aExcSet(AEXC_overstep);
        mtx_unlock(&a_ss_lock);
        return;
    }

    RAII(ALine(ASignalEnd)) local_group = A_COPY(ALine(ASignalEnd), *group);
    if(aExcOccur()){
        mtx_unlock(&a_ss_lock);
        return;
    }

    mtx_unlock(&a_ss_lock);

    a_signal_group_call(signal, &local_group);
}



void a_signal_system_register(int64_t id, void* addressee, ASignalTarget target){
    if(!a_ss_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    if(__a_unlikely(id < 0)){
        aExcSet(AEXC_overstep);
        return;
    }

    if(__a_unlikely(addressee == nullptr ||target == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    mtx_lock(&a_ss_lock);

    if(__a_unlikely(id >= a_signal_system.count)){
        aExcSet(AEXC_overstep);
        mtx_unlock(&a_ss_lock);
        return;
    }

    auto tab = &a_signal_system.tab;
    auto group = tab->f->at(tab, (uint32_t)id);
    if(__a_unlikely(group == nullptr)){
        aExcSet(AEXC_overstep);
        mtx_unlock(&a_ss_lock);
        return;
    }

    forEach(it, *group){
        if(A_CMPD(cptr_t, it.p->addressee, addressee) == 0){
            aExcSet(AEXC_repeat_write);
            mtx_unlock(&a_ss_lock);
            return;
        }
    }

    group->f->pushBack(group, (ASignalEnd){ addressee, target });

    mtx_unlock(&a_ss_lock);
}



void a_signal_system_unregister(int64_t id, void* addressee){
    if(!a_ss_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    if(__a_unlikely(id < 0)){
        aExcSet(AEXC_overstep);
        return;
    }

    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    mtx_lock(&a_ss_lock);

    if(__a_unlikely(id >= a_signal_system.count)){
        aExcSet(AEXC_overstep);
        mtx_unlock(&a_ss_lock);
        return;
    }

    auto tab = &a_signal_system.tab;
    auto group = tab->f->at(tab, (uint32_t)id);
    if(__a_unlikely(group == nullptr)){
        aExcSet(AEXC_overstep);
        mtx_unlock(&a_ss_lock);
        return;
    }

    for(uint32_t i = 0; i < group->f->getNumber(group); i++){
        auto end = group->f->at(group, i);
        if(A_CMPD(cptr_t, end->addressee, addressee) == 0){
            group->f->rm(group, i); break;
        }
    }

    mtx_unlock(&a_ss_lock);
}





