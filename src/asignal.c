/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <aline.h>
#include <ahash.h>
#include <asignal.h>
#include <threads.h>

/* 信号接收端 */
typedef struct{
    /* 信号接收者 */
    void* addressee;
    /* 信号靶函数 */
    void (*call)(const ASignal* signal, void* addressee);
}ATarget;
A_TYPE_REGISTER(ATarget);



ALine_Define(Aint);
ALine_Generate(Aint);
A_TYPE_REGISTER(ALine(Aint));

ALine_Define(ASignal);
ALine_Generate(ASignal);
A_TYPE_REGISTER(ALine(ASignal));

ALine_Define(ATarget);
ALine_Generate(ATarget);
A_TYPE_REGISTER(ALine(ATarget));



/* 信号连接体 */
/* 用于描述一个信号连接的目标 */
typedef struct{
    Aint            id;
    ALine(ATarget)  tar_list;
}AConnector;
static void A_OBJ_INIT(AConnector)(AConnector* self){
    self->id = 0; self->tar_list = A_INIT(ALine(ATarget));
}
static void A_OBJ_DEST(AConnector)(AConnector* self){
    self->id = 0; A_DEST(ALine(ATarget), self->tar_list);
}
static void A_OBJ_COPY(AConnector)(AConnector* self, const AConnector* that){
    self->id = that->id; self->tar_list = A_COPY(ALine(ATarget), that->tar_list);
}
static int A_OBJ_CMPD(AConnector)(const AConnector* self, const AConnector* that){
    int ret = A_CMPD(Aint, self->id, that->id);
    return ret != 0 ? ret : A_CMPD(ALine(ATarget), self->tar_list, that->tar_list);
}
A_TYPE_REGISTER(AConnector);

static inline void AConnector_add(AConnector* self, void* addressee, void(*call)(const ASignal*, void*)){
    ATarget tar = {.addressee = addressee, .call = call };
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(addressee == nullptr || call == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }
    auto list = &self->tar_list;
    forEach(it, *list){
        if(it.p->addressee == addressee){
            //重复注册
            aExcSet(AEXC_repeat_write);
            return;
        }
    }
    list->f->pushBack(list, tar);
}
static inline void AConnector_rm(AConnector* self, void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    auto list = &self->tar_list;
    for(uint32_t i = 0; i < list->f->getNumber(list); i++){
        auto tar = list->f->at(list, i);
        if(tar != nullptr && tar->addressee == addressee){
            list->f->rm(list, i);
            break;
        }
    }
}
static inline bool AConnector_empty(const AConnector* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->tar_list;
    return list->f->empty(list);
}



AHash_Define(Aint,AConnector);
AHash_Generate(Aint,AConnector);
A_TYPE_REGISTER(AHash(Aint,AConnector));



/* 目标连接体 */
/* 用于描述一个对象连接的信号 */
typedef struct{
    void*           addressee;
    ALine(Aint)     id_list;
}ATargetConnector;
static void A_OBJ_INIT(ATargetConnector)(ATargetConnector* self){
    self->addressee = nullptr; self->id_list = A_INIT(ALine(Aint));
}
static void A_OBJ_DEST(ATargetConnector)(ATargetConnector* self){
    self->addressee = nullptr; A_DEST(ALine(Aint), self->id_list);
}
static void A_OBJ_COPY(ATargetConnector)(ATargetConnector* self, const ATargetConnector* that){
    self->addressee = that->addressee; self->id_list = A_COPY(ALine(Aint), that->id_list);
}
static int A_OBJ_CMPD(ATargetConnector)(const ATargetConnector* self, const ATargetConnector* that){
    int ret = A_CMPD(cptr_t, self->addressee, that->addressee);
    return ret != 0 ? ret : A_CMPD(ALine(Aint), self->id_list, that->id_list);
}
A_TYPE_REGISTER(ATargetConnector);

static inline void ATargetConnector_add(ATargetConnector* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return;
    }
    auto list = &self->id_list;
    forEach(it, *list){
        if(*it.p == id){
            //重复注册
            aExcSet(AEXC_repeat_write);
            return;
        }
    }
    list->f->pushBack(list, id);
}
static inline void ATargetConnector_rm(ATargetConnector* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return;
    }

    auto list = &self->id_list;
    for(uint32_t i = 0; i < list->f->getNumber(list); i++){
        auto idp = list->f->at(list, i);
        if(idp != nullptr && *idp == id){
            list->f->rm(list, i);
            break;
        }
    }
}
static inline bool ATargetConnector_exist(const ATargetConnector* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return false;
    }

    auto list = &self->id_list;
    forEach(it, *list){
        if(*it.p == id) return true;
    }

    return false;
}
static inline bool ATargetConnector_empty(const ATargetConnector* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->id_list;
    return list->f->empty(list);
}



AHash_Define(cptr_t,ATargetConnector);
AHash_Generate(cptr_t,ATargetConnector);
A_TYPE_REGISTER(AHash(cptr_t,ATargetConnector));



/* 信号系统 */
typedef struct{
    Aint                            count;
    AHash(Aint,AConnector)          connector_table;
    AHash(cptr_t,ATargetConnector)  addressee_table;
}ASignalSystem;
static void A_OBJ_INIT(ASignalSystem)(ASignalSystem* self){
    self->count = 1;
    self->connector_table = A_INIT(AHash(Aint, AConnector));
    self->addressee_table = A_INIT(AHash(cptr_t,ATargetConnector));
}
static void A_OBJ_DEST(ASignalSystem)(ASignalSystem* self){
    self->count = 0;
    A_DEST(AHash(Aint,AConnector), self->connector_table);
    A_DEST(AHash(cptr_t,ATargetConnector), self->addressee_table);
}
static void A_OBJ_COPY(ASignalSystem)(ASignalSystem* self, const ASignalSystem* that){
    self->count = that->count;
    self->connector_table = A_COPY(AHash(Aint,AConnector), that->connector_table);
    self->addressee_table = A_COPY(AHash(cptr_t,ATargetConnector), that->addressee_table);
}
static int A_OBJ_CMPD(ASignalSystem)(const ASignalSystem* self, const ASignalSystem* that){
    int ret = A_CMPD(Aint, self->count, that->count);
    if(ret == 0) ret = A_CMPD(AHash(Aint,AConnector), self->connector_table, that->connector_table);
    if(ret == 0) ret = A_CMPD(AHash(cptr_t,ATargetConnector), self->addressee_table, that->addressee_table);
    return ret;
}
A_TYPE_REGISTER(ASignalSystem);

static inline Aint ASignalSystem_alloc(ASignalSystem* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }
    return self->count++;
}
static inline ATargetConnector* ASignalSystem_find_forad(const ASignalSystem* self, void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    auto map = &self->addressee_table;
    auto connector  = map->f->at(map, addressee);

    if(aExcOccur() && aExcGet() == AEXC_overstep){
        aExcClean();
    }
    return connector;
}
static inline AConnector* ASignalSystem_find_forid(const ASignalSystem* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }
    if(__a_unlikely(id <= 0 || id >= self->count)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    auto map = &self->connector_table;
    auto connector = map->f->at(map, id);

    if(aExcOccur() && aExcGet() == AEXC_overstep){
        aExcClean();
    }
    return connector;
}
static inline void ASignalSystem_add(ASignalSystem* self, Aint id, void* addressee, void(*call)(const ASignal*, void*)){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(id <= 0 || id >= self->count || addressee == nullptr || call == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    aExcClean();

    auto cmap = &self->connector_table;
    auto cval = cmap->f->at(cmap, id);
    if(cval == nullptr){
        RAII(AConnector) con = A_INIT(AConnector);
        if(aExcOccur()){
            return;
        }
        con.id = id;
        auto tab = &self->connector_table;
        tab->f->ins(tab, id, con);
        if(aExcOccur()){
            return;
        }
    }
    auto amap = &self->addressee_table;
    auto aval = amap->f->at(amap, addressee);
    if(aval == nullptr){
        RAII(ATargetConnector) con = A_INIT(ATargetConnector);
        if(aExcOccur()){
            return;
        }
        con.addressee = addressee;
        auto tab = &self->addressee_table;
        tab->f->ins(tab, addressee, con);
        if(aExcOccur()){
            return;
        }
    }

    auto id_con = ASignalSystem_find_forid(self, id);
    if(id_con == nullptr){
        aExcSet(AEXC_outdomain);
        return;
    }
    AConnector_add(id_con, addressee, call);
    if(aExcOccur()){
        return;
    }

    auto ad_con = ASignalSystem_find_forad(self, addressee);
    if(ad_con == nullptr){
        aExcSet(AEXC_outdomain);
        return;
    }
    ATargetConnector_add(ad_con, id);
    if(aExcOccur()){
        AConnector_rm(id_con, addressee);
        return;
    }
}
static inline void ASignalSystem_rm_one(ASignalSystem* self, Aint id, void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(id <= 0 || id >= self->count || addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    aExcClean();

    {
        auto connector = ASignalSystem_find_forid(self, id);
        if(connector != nullptr){
            AConnector_rm(connector, addressee);
            if(AConnector_empty(connector)){
                auto tab = &self->connector_table;
                tab->f->rm(tab, id);
            }
        }
    }
    {
        auto connector = ASignalSystem_find_forad(self, addressee);
        if(connector != nullptr){
            ATargetConnector_rm(connector, id);
            if(ATargetConnector_empty(connector)){
                auto tab = &self->addressee_table;
                tab->f->rm(tab, addressee);
            }
        }
    }
}
static inline void ASignalSystem_rm_forid(ASignalSystem* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(id <= 0 || id >= self->count)){
        aExcSet(AEXC_outdomain);
        return;
    }

    aExcClean();

    auto tab = &self->connector_table;
    RAII(AConnector) connector; tab->f->take(tab, id, &connector);
    if(aExcOccur()){
        if(aExcGet() == AEXC_overstep){
            aExcClean();
        }
        return;
    }

    forEach(it, connector.tar_list){
        auto addressee = it.p->addressee;
        ASignalSystem_rm_one(self, id, addressee);
    }
}
static inline void ASignalSystem_rm_forad(ASignalSystem* self, void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    aExcClean();

    auto tab = &self->addressee_table;
    RAII(ATargetConnector) connector; tab->f->take(tab, addressee, &connector);
    if(aExcOccur()){
        if(aExcGet() == AEXC_overstep){
            aExcClean();
        }
        return;
    }

    forEach(it, connector.id_list){
        auto id = *it.p;
        ASignalSystem_rm_one(self, id, addressee);
    }
}
static inline bool ASignalSystem_exist(ASignalSystem* self, Aint id, void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    if(__a_unlikely(addressee == nullptr || id <= 0 || id >= self->count)){
        aExcSet(AEXC_outdomain);
        return false;
    }

    aExcClean();

    auto connector = ASignalSystem_find_forad(self, addressee);
    if(connector == nullptr) return false;

    return ATargetConnector_exist(connector, id);
}



typedef struct{
    mtx_t* lock;
}AMtxCtrl;
static void A_OBJ_DEST(AMtxCtrl)(AMtxCtrl* self){
    if(__a_likely(self->lock != nullptr)){
        mtx_unlock(self->lock), self->lock = nullptr;
    }
}
static void A_OBJ_COPY(AMtxCtrl)(AMtxCtrl* self, __unused const AMtxCtrl* that){
    self->lock = nullptr;
}
A_TYPE_REGISTER(AMtxCtrl);
static inline AMtxCtrl AMtxCtrl_lock(mtx_t* lock){
    if(__a_likely(lock != nullptr)){
        if(mtx_lock(lock) != thrd_success){
            aExcSet(AEXC_system_error);
            lock = nullptr;
        }
    }
    return (AMtxCtrl){ .lock = lock };
}



static mtx_t a_system_lock;

static bool a_system_flag = false;

static ASignalSystem a_signal_system;

__attribute__((constructor)) static inline void a_signal_system_start(){
    aExcClean();

    if(mtx_init(&a_system_lock, mtx_plain | mtx_recursive) != thrd_success) {
        aExcSet(AEXC_system_error);
        return;
    }

    a_signal_system = A_INIT(ASignalSystem);

    if(aExcOccur()){
        mtx_destroy(&a_system_lock);
        a_system_flag = false;
    }else{
        a_system_flag = true;
    }
}
__attribute__((destructor)) static inline void a_signal_system_poweroff(){
    if(a_system_flag){
        A_DEST(ASignalSystem, a_signal_system);
        mtx_destroy(&a_system_lock);
        a_system_flag = false;
    }
}



Aint a_signal_alloc(){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return -1;
    }

    aExcClean();
    RAII(AMtxCtrl) amtxctrl = AMtxCtrl_lock(&a_system_lock);
    if(aExcOccur()){
        return -1;
    }

    aExcClean();
    auto id = ASignalSystem_alloc(&a_signal_system);
    if(aExcOccur()){
        id = -1;
    }

    return id;
}

void a_signal_transmit(const ASignal* signal){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    if(__a_unlikely(signal == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto id = signal->id;

    aExcClean();
    RAII(AMtxCtrl) amtxctrl = AMtxCtrl_lock(&a_system_lock);
    if(aExcOccur()){
        return;
    }

    auto connector = ASignalSystem_find_forid(&a_signal_system, id);
    if(connector == nullptr){
        return;
    }

    RAII(AConnector) con = A_COPY(AConnector, *connector);
    if(aExcOccur()){
        return;
    }

    forEach(it, con.tar_list){
        auto call = it.p->call; auto addressee = it.p->addressee;
        call(signal, addressee);
        aExcClean();
    }
}

void a_signal_connection(Aint id, const void* addressee, void(*call)(const ASignal*, void*)){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    aExcClean();
    RAII(AMtxCtrl) amtxctrl = AMtxCtrl_lock(&a_system_lock);
    if(aExcOccur()){
        return;
    }

    ASignalSystem_add(&a_signal_system, id, (void*)addressee, call);
}

void a_signal_disconnect(Aint id, const void* addressee){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    aExcClean();
    RAII(AMtxCtrl) amtxctrl = AMtxCtrl_lock(&a_system_lock);
    if(aExcOccur()){
        return;
    }

    ASignalSystem_rm_one(&a_signal_system, id, (void*)addressee);
}

void a_target_disconnect(const void* addressee, Aint id){
    a_signal_disconnect(id, addressee);
}

void a_signal_disconnect_all(Aint id){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    aExcClean();
    RAII(AMtxCtrl) amtxctrl = AMtxCtrl_lock(&a_system_lock);
    if(aExcOccur()){
        return;
    }

    ASignalSystem_rm_forid(&a_signal_system, id);
}

void a_target_disconnect_all(const void* addressee){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    aExcClean();
    RAII(AMtxCtrl) amtxctrl = AMtxCtrl_lock(&a_system_lock);
    if(aExcOccur()){
        return;
    }

    ASignalSystem_rm_forad(&a_signal_system, (void*)addressee);
}

