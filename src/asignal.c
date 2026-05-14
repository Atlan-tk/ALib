/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <aline.h>
#include <ahash.h>
#include <alock.h>
#include <asignal.h>
#include <stdatomic.h>

void ASignal_setExcList(ASignal* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    auto list = self->exc_list;
    if(list == nullptr){
        self->exc_list = A_NEW(ALine(AResponseExc));
    }
}
void ASignal_cleanExc(const ASignal* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto list = self->exc_list;
    if(list != nullptr && list->f->getNumber(list) != 0){
        A_DEST(ALine(AResponseExc), *list);
        *list = A_INIT(ALine(AResponseExc));
    }

    return;
}
AResponseExc ASignal_popExc(const ASignal* self){
    AResponseExc ret = {};

    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return ret;
    }

    auto list = self->exc_list;
    if(list != nullptr){
        list->f->popBack(list, &ret);
    }

    return ret;
}

/* 异常收集 */
static inline void ASignal_exc_collect(ASignal* self, void* addressee, AEXC_t ev){
    auto list = self->exc_list;
    if(__a_unlikely(list == nullptr)) return;

    aExcClean();
    list->f->pushBack(list, (AResponseExc){ addressee, ev});
    if(aExcOccur()){
        aExcSet(AEXC_collect_failed);
    }
}
/* 异常列表是否为空 */
static inline bool ASignal_exc_empty(const ASignal* self){
    auto list = self->exc_list;
    if(__a_unlikely(list == nullptr)) return true;
    return list->f->empty(list);
}



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
}ASignalTower;
static void A_OBJ_INIT(ASignalTower)(ASignalTower* self){
    self->id = 0; self->tar_list = A_INIT(ALine(ATarget));
}
static void A_OBJ_DEST(ASignalTower)(ASignalTower* self){
    self->id = 0; A_DEST(ALine(ATarget), self->tar_list);
}
static void A_OBJ_COPY(ASignalTower)(ASignalTower* self, const ASignalTower* that){
    self->id = that->id; self->tar_list = A_COPY(ALine(ATarget), that->tar_list);
}
static int A_OBJ_CMPD(ASignalTower)(const ASignalTower* self, const ASignalTower* that){
    int ret = A_CMPD(Aint, self->id, that->id);
    return ret != 0 ? ret : A_CMPD(ALine(ATarget), self->tar_list, that->tar_list);
}
A_TYPE_REGISTER(ASignalTower);

static inline void ASignalTower_add(ASignalTower* self, void* addressee, void(*call)(const ASignal*, void*)){
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
static inline void ASignalTower_rm(ASignalTower* self, void* addressee){
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
static inline bool ASignalTower_empty(const ASignalTower* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->tar_list;
    return list->f->empty(list);
}



AHash_Define(Aint,ASignalTower);
AHash_Generate(Aint,ASignalTower);
A_TYPE_REGISTER(AHash(Aint,ASignalTower));



/* 目标连接体 */
/* 用于描述一个对象连接的信号 */
typedef struct{
    void*           addressee;
    ALine(Aint)     id_list;
}ASignalRadio;
static void A_OBJ_INIT(ASignalRadio)(ASignalRadio* self){
    self->addressee = nullptr; self->id_list = A_INIT(ALine(Aint));
}
static void A_OBJ_DEST(ASignalRadio)(ASignalRadio* self){
    self->addressee = nullptr; A_DEST(ALine(Aint), self->id_list);
}
static void A_OBJ_COPY(ASignalRadio)(ASignalRadio* self, const ASignalRadio* that){
    self->addressee = that->addressee; self->id_list = A_COPY(ALine(Aint), that->id_list);
}
static int A_OBJ_CMPD(ASignalRadio)(const ASignalRadio* self, const ASignalRadio* that){
    int ret = A_CMPD(cptr_t, self->addressee, that->addressee);
    return ret != 0 ? ret : A_CMPD(ALine(Aint), self->id_list, that->id_list);
}
A_TYPE_REGISTER(ASignalRadio);

static inline void ASignalRadio_add(ASignalRadio* self, Aint id){
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
static inline void ASignalRadio_rm(ASignalRadio* self, Aint id){
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
static inline bool ASignalRadio_exist(const ASignalRadio* self, Aint id){
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
static inline bool ASignalRadio_empty(const ASignalRadio* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->id_list;
    return list->f->empty(list);
}



AHash_Define(cptr_t,ASignalRadio);
AHash_Generate(cptr_t,ASignalRadio);
A_TYPE_REGISTER(AHash(cptr_t,ASignalRadio));



/* 信号系统 */
typedef struct{
    atomic_long                 count;
    AHash(Aint,ASignalTower)    connector_table;
    AHash(cptr_t,ASignalRadio)  addressee_table;
}ASignalSystem;
static void A_OBJ_INIT(ASignalSystem)(ASignalSystem* self){
    atomic_store_explicit(&self->count, 1, memory_order_relaxed);
    self->connector_table = A_INIT(AHash(Aint, ASignalTower));
    self->addressee_table = A_INIT(AHash(cptr_t,ASignalRadio));
}
static void A_OBJ_DEST(ASignalSystem)(ASignalSystem* self){
    atomic_store_explicit(&self->count, 0, memory_order_relaxed);
    A_DEST(AHash(Aint,ASignalTower), self->connector_table);
    A_DEST(AHash(cptr_t,ASignalRadio), self->addressee_table);
}
static void A_OBJ_COPY(ASignalSystem)(ASignalSystem* self, const ASignalSystem* that){
    self->count = that->count;
    self->connector_table = A_COPY(AHash(Aint,ASignalTower), that->connector_table);
    self->addressee_table = A_COPY(AHash(cptr_t,ASignalRadio), that->addressee_table);
}
static int A_OBJ_CMPD(ASignalSystem)(const ASignalSystem* self, const ASignalSystem* that){
    int ret = A_CMPD(Aint, (Aint)self->count, (Aint)that->count);
    if(ret == 0) ret = A_CMPD(AHash(Aint,ASignalTower), self->connector_table, that->connector_table);
    if(ret == 0) ret = A_CMPD(AHash(cptr_t,ASignalRadio), self->addressee_table, that->addressee_table);
    return ret;
}
A_TYPE_REGISTER(ASignalSystem);

static inline Aint ASignalSystem_alloc(ASignalSystem* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }
    return atomic_fetch_add(&self->count, 1);
}
static inline ASignalRadio* ASignalSystem_find_forad(const ASignalSystem* self, void* addressee){
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
static inline ASignalTower* ASignalSystem_find_forid(const ASignalSystem* self, Aint id){
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
        RAII(ASignalTower) con = A_INIT(ASignalTower);
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
        RAII(ASignalRadio) con = A_INIT(ASignalRadio);
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
    ASignalTower_add(id_con, addressee, call);
    if(aExcOccur()){
        return;
    }

    auto ad_con = ASignalSystem_find_forad(self, addressee);
    if(ad_con == nullptr){
        aExcSet(AEXC_outdomain);
        return;
    }
    ASignalRadio_add(ad_con, id);
    if(aExcOccur()){
        ASignalTower_rm(id_con, addressee);
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
            ASignalTower_rm(connector, addressee);
            if(ASignalTower_empty(connector)){
                auto tab = &self->connector_table;
                tab->f->rm(tab, id);
            }
        }
    }
    {
        auto connector = ASignalSystem_find_forad(self, addressee);
        if(connector != nullptr){
            ASignalRadio_rm(connector, id);
            if(ASignalRadio_empty(connector)){
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
    RAII(ASignalTower) connector; tab->f->take(tab, id, &connector);
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
    RAII(ASignalRadio) connector; tab->f->take(tab, addressee, &connector);
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
__unused static inline bool ASignalSystem_exist(ASignalSystem* self, Aint id, void* addressee){
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

    return ASignalRadio_exist(connector, id);
}



static AMtxRW a_call_lock;
static AMtxRW a_global_lock;

static bool a_system_flag = false;

/* 防止同线程递归加锁 */
static thread_local int  a_call_num = 0;

static ASignalSystem a_signal_system;

__attribute__((constructor)) static inline void a_signal_system_start(){
    aExcClean();

    a_call_lock = A_INIT(AMtxRW);
    if(aExcOccur()){
        return;
    }

    a_global_lock = A_INIT(AMtxRW);
    if(aExcOccur()){
        A_DEST(AMtxRW, a_call_lock);
        return;
    }

    a_signal_system = A_INIT(ASignalSystem);

    if(aExcOccur()){
        A_DEST(AMtxRW, a_global_lock);
        A_DEST(AMtxRW, a_call_lock);
        a_system_flag = false;
    }else{
        a_system_flag = true;
    }
}
__attribute__((destructor)) static inline void a_signal_system_poweroff(){
    if(a_system_flag){
        A_DEST(ASignalSystem, a_signal_system);
        A_DEST(AMtxRW, a_global_lock);
        A_DEST(AMtxRW, a_call_lock);
        a_system_flag = false;
    }
}



Aint a_signal_alloc(){
    aExcClean();

    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return -1;
    }

    return ASignalSystem_alloc(&a_signal_system);
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
    RAII(AAutoKey) autokey_c = AMtxRW_rlock(&a_call_lock);
    if(aExcOccur()){
        return;
    }
    RAII(AAutoKey) autokey_g = AMtxRW_rlock(&a_global_lock);
    if(aExcOccur()){
        return;
    }

    auto connector = ASignalSystem_find_forid(&a_signal_system, id);
    if(connector == nullptr){
        return;
    }

    RAII(ASignalTower) tower = A_COPY(ASignalTower, *connector);
    if(aExcOccur()){
        return;
    }

    A_DEST(AAutoKey, autokey_g);

    if(a_call_num == 0){
        signal->f->cleanExc(signal);
    }
    a_call_num++;

    do{
        aExcClean();

        forEach(it, tower.tar_list){
            auto call = it.p->call; auto addressee = it.p->addressee;
            call(signal, addressee);

            if(aExcOccur()){
                AEXC_t ev = aExcGet(); aExcClean();
                ASignal_exc_collect((void*)signal, addressee, ev);
                if(aExcOccur()){
                    break;
                }
            }
        }

        if(!ASignal_exc_empty(signal)){
            aExcSet(AEXC_response_exc);
        }
    }while(0);
    a_call_num--;
}

void a_signal_connection(Aint id, const void* addressee, void(*call)(const ASignal*, void*)){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    aExcClean();
    RAII(AAutoKey) autokey = AMtxRW_wlock(&a_global_lock);
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

    if(a_call_num != 0){
        aExcSet(AEXC_repeat_write);
        return;
    }

    aExcClean();
    RAII(AAutoKey) autokey_c = AMtxRW_wlock(&a_call_lock);
    if(aExcOccur()){
        return;
    }
    RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
    if(aExcOccur()){
        return;
    }

    ASignalSystem_rm_one(&a_signal_system, id, (void*)addressee);
}

void a_signal_disconnect_all(Aint id){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    if(a_call_num != 0){
        aExcSet(AEXC_repeat_write);
        return;
    }

    aExcClean();
    RAII(AAutoKey) autokey_c = AMtxRW_wlock(&a_call_lock);
    if(aExcOccur()){
        return;
    }
    RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
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

    if(a_call_num != 0){
        aExcSet(AEXC_repeat_write);
        return;
    }

    aExcClean();
    RAII(AAutoKey) autokey_c = AMtxRW_wlock(&a_call_lock);
    if(aExcOccur()){
        return;
    }
    RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
    if(aExcOccur()){
        return;
    }

    ASignalSystem_rm_forad(&a_signal_system, (void*)addressee);
}

