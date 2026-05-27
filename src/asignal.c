/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <aline.h>
#include <alist.h>
#include <ahash.h>
#include <alock.h>
#include <asignal.h>
#include <stdatomic.h>

typedef const void* const_ptr_t;
A_TYPE_REGISTER(const_ptr_t);



AExcEnd AExcCollector_pop(AExcCollector* self){
    AExcEnd ret = {};
    if(__a_unlikely(self == nullptr)){
        return ret;
    }
    auto list = &self->list;
    list->f->popBack(list, &ret);
    return ret;
}

/* 异常收集 */
void AExcCollector_collect(AExcCollector* self, const void* addressee, AEXC_t ev){
    if(__a_unlikely(self == nullptr)){
        return;
    }
    if(ev != AEXC_NORMAL && ev != AEXC_matching_failed){
        auto list = &self->list; aExcClean();
        list->f->pushBack(list, (AExcEnd){ addressee, ev});
        self->exc = aExcGet(); aExcClean();
    }
}
/* 异常列表是否为空 */
bool AExcCollector_empty(const AExcCollector* self){
    if(__a_unlikely(self == nullptr)){
        return true;
    }
    auto list = &self->list;
    auto ret = list->f->empty(list);
    return ret;
}

uint32_t AExcCollector_getNumber(const AExcCollector* self){
    if(__a_unlikely(self == nullptr)){
        return 0;
    }
    auto list = &self->list;
    auto ret =  list->f->getNumber(list);
    return ret;
}



/* 连接描述 */
typedef struct{
    Aint        id;
    const void* addressee;
    void        (*call)(const ASignal* signal, void* addressee);
    uint32_t    call_num;
    bool        dis_request;
}ALink;
A_TYPE_REGISTER(ALink);

typedef ALink* ALinkp;
A_TYPE_REGISTER(ALinkp);

AList_Define(ALink);
AList_Generate(ALink);
A_TYPE_REGISTER(AList(ALink));

ALine_Define(ALinkp);
ALine_Generate(ALinkp);
A_TYPE_REGISTER(ALine(ALinkp));



/* 信号连接体 */
typedef struct{
    Aint            id;
    ALine(ALinkp)   linkpList;
}ASignalTower;
__noused __weak void A_OBJ_INIT(ASignalTower)(ASignalTower* self){
    self->id = 0; self->linkpList = A_INIT(ALine(ALinkp));
}
__noused __weak void A_OBJ_DEST(ASignalTower)(ASignalTower* self){
    self->id = 0; A_DEST(ALine(ALinkp), self->linkpList);
}
__noused __weak void A_OBJ_COPY(ASignalTower)(ASignalTower* self, const ASignalTower* that){
    self->id = that->id; self->linkpList = A_COPY(ALine(ALinkp), that->linkpList);
}
__noused __weak int A_OBJ_CMPD(ASignalTower)(const ASignalTower* self, const ASignalTower* that){
    int ret = A_CMPD(Aint, self->id, that->id);
    return ret != 0 ? ret : A_CMPD(ALine(ALinkp), self->linkpList, that->linkpList);
}
A_TYPE_REGISTER(ASignalTower);

static inline uint32_t ASignalTower_find_i(const ASignalTower* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0xffffffff;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return 0xffffffff;
    }

    auto list = &self->linkpList;
    for(uint32_t i = 0; i < list->f->getNumber(list); i++){
        auto pp = list->f->at(list, i);
        if(pp != nullptr && *pp != nullptr && (*pp)->addressee == addressee){
            return i;
        }
    }
    return 0xffffffff;
}
static inline void ASignalTower_add(ASignalTower* self, ALink* p){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(p == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    uint32_t i = ASignalTower_find_i(self, p->addressee);
    auto list = &self->linkpList;
    if(i == 0xffffffff){
        list->f->pushBack(list, p);
    }
}
static inline void ASignalTower_rm(ASignalTower* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    uint32_t i = ASignalTower_find_i(self, addressee);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        list->f->rm(list, i);
    }
}
static inline ALink* ASignalTower_find(const ASignalTower* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    uint32_t i = ASignalTower_find_i(self, addressee);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        auto pp = list->f->at(list, i);
        if(pp != nullptr) return *pp;
    }

    return nullptr;
}
static inline bool ASignalTower_exist(const ASignalTower* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return false;
    }

    uint32_t i = ASignalTower_find_i(self, addressee);
    if(i != 0xffffffff){
        return true;
    }

    return false;
}
static inline bool ASignalTower_empty(const ASignalTower* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->linkpList;
    return list->f->empty(list);
}
static inline int ASignalTower_call(const ASignalTower* self, const ASignal* signal, AExcCollector* collector){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    if(__a_unlikely(signal == nullptr || signal->id <= 0)){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = 0;
    auto list = &self->linkpList;
    forEach(it, *list){
        auto link = *it.p;
        if(__a_unlikely(link->id != signal->id)){
            aExcSet(AEXC_system_error);
            return 0;
        }

        aExcClean();
        if(!link->dis_request) link->call(signal, (void*)(link->addressee));
        if(aExcOccur()){
            ret = AEXC_response_exc;
            AEXC_t ev = aExcGet(); aExcClean();
            AExcCollector_collect(collector, link->addressee, ev);
            if(collector && collector->exc){
                break;
            }
        }

    }
    return ret;
}

AHash_Define(Aint,ASignalTower);
AHash_Generate(Aint,ASignalTower);
A_TYPE_REGISTER(AHash(Aint,ASignalTower));



/* 目标连接体 */
typedef struct{
    const void*     addressee;
    ALine(ALinkp)   linkpList;
}ASignalRadio;
__noused __weak void A_OBJ_INIT(ASignalRadio)(ASignalRadio* self){
    self->addressee = nullptr; self->linkpList = A_INIT(ALine(ALinkp));
}
__noused __weak void A_OBJ_DEST(ASignalRadio)(ASignalRadio* self){
    self->addressee = nullptr; A_DEST(ALine(ALinkp), self->linkpList);
}
__noused __weak void A_OBJ_COPY(ASignalRadio)(ASignalRadio* self, const ASignalRadio* that){
    self->addressee = that->addressee; self->linkpList = A_COPY(ALine(ALinkp), that->linkpList);
}
__noused __weak int A_OBJ_CMPD(ASignalRadio)(const ASignalRadio* self, const ASignalRadio* that){
    int ret = A_CMPD(const_ptr_t, (const_ptr_t)(self->addressee), (const_ptr_t)(that->addressee));
    return ret != 0 ? ret : A_CMPD(ALine(ALinkp), self->linkpList, that->linkpList);
}
A_TYPE_REGISTER(ASignalRadio);

static inline uint32_t ASignalRadio_find_i(const ASignalRadio* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0xffffffff;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return 0xffffffff;
    }

    auto list = &self->linkpList;
    for(uint32_t i = 0; i < list->f->getNumber(list); i++){
        auto pp = list->f->at(list, i);
        if(pp != nullptr && (*pp)->id == id){
            return i;
        }
    }
    return 0xffffffff;
}
static inline void ASignalRadio_add(ASignalRadio* self, ALink* p){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(p == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    uint32_t i = ASignalRadio_find_i(self, p->id);
    auto list = &self->linkpList;
    if(i == 0xffffffff){
        list->f->pushBack(list, p);
    }
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

    uint32_t i = ASignalRadio_find_i(self, id);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        list->f->rm(list, i);
    }
}
__noused static inline ALink* ASignalRadio_find(const ASignalRadio* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    uint32_t i = ASignalRadio_find_i(self, id);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        auto pp = list->f->at(list, i);
        if(pp != nullptr) return *pp;
    }

    return nullptr;
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

    uint32_t i = ASignalRadio_find_i(self, id);
    if(i != 0xffffffff){
        return true;
    }

    return false;
}
static inline bool ASignalRadio_empty(const ASignalRadio* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->linkpList;
    return list->f->empty(list);
}

AHash_Define(const_ptr_t,ASignalRadio);
AHash_Generate(const_ptr_t,ASignalRadio);
A_TYPE_REGISTER(AHash(const_ptr_t,ASignalRadio));



/* 信号系统 */
typedef struct{
#if __SIZEOF_POINTER__ == 8
    atomic_llong                count;
#else
    atomic_long                 count;
#endif /* 64 or 32 */
    AList(ALink)                linkPool;
    AHash(Aint,ASignalTower)    idMap;
    AHash(const_ptr_t,ASignalRadio)  adMap;
}ASignalSystem;
__noused __weak void A_OBJ_INIT(ASignalSystem)(ASignalSystem* self){
    aExcClean();
    atomic_store_explicit(&self->count, 1, memory_order_relaxed);
    self->linkPool = A_INIT(AList(ALink));
    if(aExcOccur()) return;
    self->idMap = A_INIT(AHash(Aint, ASignalTower));
    if(aExcOccur()) return;
    self->adMap = A_INIT(AHash(const_ptr_t,ASignalRadio));
    if(aExcOccur()) return;
}
__noused __weak void A_OBJ_DEST(ASignalSystem)(ASignalSystem* self){
    atomic_store_explicit(&self->count, 0, memory_order_relaxed);
    A_DEST(AList(ALink), self->linkPool);
    A_DEST(AHash(Aint,ASignalTower), self->idMap);
    A_DEST(AHash(const_ptr_t,ASignalRadio), self->adMap);
}
__noused __weak void A_OBJ_COPY(ASignalSystem)(ASignalSystem* self, __noused const ASignalSystem* that){
    memset(self, 0, sizeof(ASignalSystem));
}
__noused __weak int A_OBJ_CMPD(ASignalSystem)(const ASignalSystem* self, const ASignalSystem* that){
    int ret = A_CMPD(Aint, (Aint)self->count, (Aint)that->count);
    if(ret == 0) ret = A_CMPD(AList(ALink), self->linkPool, that->linkPool);
    if(ret == 0) ret = A_CMPD(AHash(Aint,ASignalTower), self->idMap, that->idMap);
    if(ret == 0) ret = A_CMPD(AHash(const_ptr_t,ASignalRadio), self->adMap, that->adMap);
    return ret;
}
A_TYPE_REGISTER(ASignalSystem);

static inline Aint ASignalSystem_alloc(ASignalSystem* self){
    if(__a_unlikely(self == nullptr)){
        return -1;
    }
    return atomic_fetch_add(&self->count, 1);
}
static inline ASignalRadio* ASignalSystem_find_forad(const ASignalSystem* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    aExcClean();
    auto map = &self->adMap;
    auto radio = map->f->at(map, addressee);
    if(aExcOccur() && aExcGet() == AEXC_overstep){
        aExcClean();
    }
    return radio;
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

    aExcClean();
    auto map = &self->idMap;
    auto tower = map->f->at(map, id);
    if(aExcOccur() && aExcGet() == AEXC_overstep){
        aExcClean();
    }
    return tower;
}
static inline ALink* ASignalSystem_find(const ASignalSystem* self, Aint id, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }

    if(__a_unlikely(id <= 0 || id >= self->count || addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    auto tower = ASignalSystem_find_forid(self, id);
    if(tower == nullptr){
        return nullptr;
    }
    return ASignalTower_find(tower, addressee);
}
static inline void ASignalSystem_add(ASignalSystem* self, Aint id, const void* addressee, void(*call)(const ASignal*, void*)){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(id <= 0 || id >= self->count || addressee == nullptr || call == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    ALink* link = ASignalSystem_find(self, id, addressee);
    if(link != nullptr){
        link->call = call;
        if(link->dis_request){
            link->dis_request = false;
        }
        return;
    }

    aExcClean();

    auto idmap = &self->idMap;
    auto admap = &self->adMap;

    auto tower = idmap->f->at(idmap, id);
    if(tower == nullptr){
        RAII(ASignalTower) x = A_INIT(ASignalTower);
        if(aExcOccur()){
            return;
        }
        x.id = id;

        idmap->f->ins(idmap, id, x);
        if(aExcOccur()){
            return;
        }
        tower = idmap->f->at(idmap, id);
    }
    auto radio = admap->f->at(admap, addressee);
    if(radio == nullptr){
        RAII(ASignalRadio) x = A_INIT(ASignalRadio);
        if(aExcOccur()){
            return;
        }
        x.addressee = addressee;

        admap->f->ins(admap, addressee, x);
        if(aExcOccur()){
            return;
        }
        radio = admap->f->at(admap, addressee);
    }

    auto pool = &self->linkPool;
    {
        pool->f->pushFront(pool, (ALink){ .id = id, .addressee = addressee, .call = call });
        if(aExcOccur()){
            return;
        }
        link = pool->f->at(pool, 0);
        if(link == nullptr){
            return;
        }
    }
    if(!ASignalTower_exist(tower, addressee)){
        ASignalTower_add(tower, link);
        if(aExcOccur()){
            pool->f->rm_p(pool, link);
            return;
        }
    }
    if(!ASignalRadio_exist(radio, id)){
        ASignalRadio_add(radio, link);
        if(aExcOccur()){
            ASignalTower_rm(tower, addressee);
            pool->f->rm_p(pool, link);
            return;
        }
    }
}
static inline void ASignalSystem_rm_link(ASignalSystem* self, ALink* link){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(link == nullptr)){
        return;
    }

    if(link->call_num != 0){
        //调用期间不允许删除，仅置位dis_request
        link->dis_request = true;
        return;
    }

    auto tower = ASignalSystem_find_forid(self, link->id);
    if(tower != nullptr){
        ASignalTower_rm(tower, link->addressee);
        if(ASignalTower_empty(tower)){
            auto map = &self->idMap;
            map->f->rm(map, link->id);
        }
    }
    auto radio = ASignalSystem_find_forad(self, link->addressee);
    if(radio != nullptr){
        ASignalRadio_rm(radio, link->id);
        if(ASignalRadio_empty(radio)){
            auto map = &self->adMap;
            map->f->rm(map, link->addressee);
        }
    }

    auto pool = &self->linkPool;
    pool->f->rm_p(pool, link);
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

    auto tower_p = ASignalSystem_find_forid(self, id);
    if(tower_p != nullptr){
        RAII(ASignalTower) tower = A_COPY(ASignalTower, *tower_p);
        forEach(it, tower.linkpList){
            auto link = it.p != nullptr ? *it.p : nullptr;
            if(link != nullptr){
                ASignalSystem_rm_link(self, link);
            }
        }
    }
}
static inline void ASignalSystem_rm_forad(ASignalSystem* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    auto radio_p = ASignalSystem_find_forad(self, addressee);
    if(radio_p != nullptr){
        RAII(ASignalRadio) radio = A_COPY(ASignalRadio, *radio_p);
        forEach(it, radio.linkpList){
            auto link = it.p != nullptr ? *it.p : nullptr;
            if(link != nullptr){
                ASignalSystem_rm_link(self, link);
            }
        }
    }
}
static inline ASignalTower ASignalSystem_get_tran_list(ASignalSystem* self, const ASignal* signal){
    ASignalTower tower = {};

    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return tower;
    }

    if(__a_unlikely(signal == nullptr || signal->id <= 0 || signal->id >= self->count)){
        aExcSet(AEXC_outdomain);
        return tower;
    }

    auto id = signal->id;
    auto tower_p = ASignalSystem_find_forid(self, id);
    if(tower_p == nullptr){
        aExcSet(AEXC_outdomain);
        return tower;
    }

    aExcClean();
    tower = A_COPY(ASignalTower, *tower_p);
    if(aExcOccur()){
        return tower;
    }

    return tower;
}
static void ASignalSystem_set_call(ASignalSystem* self, ASignalTower tower){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    forEach(it, tower.linkpList){
        if(it.p != nullptr && *it.p != nullptr){
            auto link = *it.p;
            link->call_num++;
        }
    }
}
static void ASignalSystem_clean_call(ASignalSystem* self, ASignalTower tower){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    forEach(it, tower.linkpList){
        if(it.p != nullptr && *it.p != nullptr){
            auto link = *it.p;
            link->call_num--;
            if(link->dis_request){
                ASignalSystem_rm_link(self, link);
            }
        }
    }
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



Aint a_signal_alloc(void){
    aExcClean();

    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return -1;
    }

    return ASignalSystem_alloc(&a_signal_system);
}

int __a_signal_transmit(const ASignal* signal, AExcCollector* collector){
    int ret = 0;

    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return ret;
    }

    if(__a_unlikely(signal == nullptr)){
        aExcSet(AEXC_nullptr);
        return ret;
    }

    auto id = signal->id;
    if(collector != nullptr){
        collector->id = id;
        collector->sender = signal->sender;
        collector->signal_name = signal->signal_name;
    }

    aExcClean();

    RAII(AAutoKey) autokey_c = {}; if(a_call_num == 0) autokey_c = AMtxRW_rlock(&a_call_lock);
    if(aExcOccur()){
        return ret;
    }

    RAII(ASignalTower) tower = {};

    {
        RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
        if(aExcOccur()){
            return ret;
        }

        tower = ASignalSystem_get_tran_list(&a_signal_system, signal);
        if(aExcOccur()){
            return ret;
        }

        ASignalSystem_set_call(&a_signal_system, tower);
        if(aExcOccur()){
            return ret;
        }
    }

    a_call_num++;
    {
        aExcClean();
        if(ASignalTower_call(&tower, signal, collector) == AEXC_response_exc){
            ret = AEXC_response_exc;
        }
        aExcClean();
    }

    do{
        RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
        if(aExcOccur()){
            break;
        }

        ASignalSystem_clean_call(&a_signal_system, tower);
    }while(0);

    a_call_num--;
    return ret;
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
    aExcClean();

    RAII(AAutoKey) autokey_c = {}; if(a_call_num == 0) autokey_c = AMtxRW_wlock(&a_call_lock);
    if(aExcOccur()){
        return;
    }
    RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
    if(aExcOccur()){
        return;
    }

    auto link = ASignalSystem_find(&a_signal_system, id, addressee);
    if(link == nullptr){
        return;
    }

    ASignalSystem_rm_link(&a_signal_system, link);
}

void a_signal_disconnect_all(Aint id){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }
    aExcClean();

    RAII(AAutoKey) autokey_c = {}; if(a_call_num == 0) autokey_c = AMtxRW_wlock(&a_call_lock);
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
    aExcClean();

    RAII(AAutoKey) autokey_c = {}; if(a_call_num == 0) autokey_c = AMtxRW_wlock(&a_call_lock);
    if(aExcOccur()){
        return;
    }
    RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
    if(aExcOccur()){
        return;
    }

    ASignalSystem_rm_forad(&a_signal_system, addressee);
}

