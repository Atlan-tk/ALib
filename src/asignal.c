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
    int64_t     id;
    const void* addressee;
    void        (*call)(const ASignal* signal, void* addressee);
    uint32_t    call_num;
    bool        dis_request;
}__ASignalLink;
A_TYPE_REGISTER(__ASignalLink);

typedef __ASignalLink* __ASignalLinkp;
A_TYPE_REGISTER(__ASignalLinkp);

AList_Define(__ASignalLink);
AList_Generate(__ASignalLink);
A_TYPE_REGISTER(AList(__ASignalLink));

ALine_Define(__ASignalLinkp);
ALine_Generate(__ASignalLinkp);
A_TYPE_REGISTER(ALine(__ASignalLinkp));



/* 信号连接体 */
typedef struct{
    int64_t                 id;
    ALine(__ASignalLinkp)   linkpList;
}__ASignalTower;
A_TYPE_REGISTER(__ASignalTower);
__noused __visibility(hidden) void A_OBJ_INIT(__ASignalTower)(__ASignalTower* self){
    self->id = 0; self->linkpList = A_INIT(ALine(__ASignalLinkp));
}
__noused __visibility(hidden) void A_OBJ_DEST(__ASignalTower)(__ASignalTower* self){
    self->id = 0; A_DEST(ALine(__ASignalLinkp), self->linkpList);
}
__noused __visibility(hidden) void A_OBJ_COPY(__ASignalTower)(__ASignalTower* self, const __ASignalTower* that){
    self->id = that->id; self->linkpList = A_COPY(ALine(__ASignalLinkp), that->linkpList);
}
__noused __visibility(hidden) int A_OBJ_CMPD(__ASignalTower)(const __ASignalTower* self, const __ASignalTower* that){
    int ret = A_CMPD(int64_t, self->id, that->id);
    return ret != 0 ? ret : A_CMPD(ALine(__ASignalLinkp), self->linkpList, that->linkpList);
}

static inline uint32_t __ASignalTower_find_i(const __ASignalTower* self, const void* addressee){
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
static inline void __ASignalTower_add(__ASignalTower* self, __ASignalLink* p){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(p == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    uint32_t i = __ASignalTower_find_i(self, p->addressee);
    auto list = &self->linkpList;
    if(i == 0xffffffff){
        list->f->pushBack(list, p);
    }
}
static inline void __ASignalTower_rm(__ASignalTower* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    uint32_t i = __ASignalTower_find_i(self, addressee);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        list->f->rm(list, i);
    }
}
static inline __ASignalLink* __ASignalTower_find(const __ASignalTower* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    uint32_t i = __ASignalTower_find_i(self, addressee);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        auto pp = list->f->at(list, i);
        if(pp != nullptr) return *pp;
    }

    return nullptr;
}
static inline bool __ASignalTower_exist(const __ASignalTower* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }
    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return false;
    }

    uint32_t i = __ASignalTower_find_i(self, addressee);
    if(i != 0xffffffff){
        return true;
    }

    return false;
}
static inline bool __ASignalTower_empty(const __ASignalTower* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->linkpList;
    return list->f->empty(list);
}
static inline int __ASignalTower_call(const __ASignalTower* self, const ASignal* signal, AExcCollector* collector){
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

AHash_Define(int64_t,__ASignalTower);
AHash_Generate(int64_t,__ASignalTower);
A_TYPE_REGISTER(AHash(int64_t,__ASignalTower));



/* 目标连接体 */
typedef struct{
    const void*             addressee;
    ALine(__ASignalLinkp)   linkpList;
}__ASignalRadio;
A_TYPE_REGISTER(__ASignalRadio);
__noused __visibility(hidden) void A_OBJ_INIT(__ASignalRadio)(__ASignalRadio* self){
    self->addressee = nullptr; self->linkpList = A_INIT(ALine(__ASignalLinkp));
}
__noused __visibility(hidden) void A_OBJ_DEST(__ASignalRadio)(__ASignalRadio* self){
    self->addressee = nullptr; A_DEST(ALine(__ASignalLinkp), self->linkpList);
}
__noused __visibility(hidden) void A_OBJ_COPY(__ASignalRadio)(__ASignalRadio* self, const __ASignalRadio* that){
    self->addressee = that->addressee; self->linkpList = A_COPY(ALine(__ASignalLinkp), that->linkpList);
}
__noused __visibility(hidden) int A_OBJ_CMPD(__ASignalRadio)(const __ASignalRadio* self, const __ASignalRadio* that){
    int ret = A_CMPD(const_ptr_t, (const_ptr_t)(self->addressee), (const_ptr_t)(that->addressee));
    return ret != 0 ? ret : A_CMPD(ALine(__ASignalLinkp), self->linkpList, that->linkpList);
}

static inline uint32_t __ASignalRadio_find_i(const __ASignalRadio* self, int64_t id){
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
static inline void __ASignalRadio_add(__ASignalRadio* self, __ASignalLink* p){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(p == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    uint32_t i = __ASignalRadio_find_i(self, p->id);
    auto list = &self->linkpList;
    if(i == 0xffffffff){
        list->f->pushBack(list, p);
    }
}
static inline void __ASignalRadio_rm(__ASignalRadio* self, int64_t id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return;
    }

    uint32_t i = __ASignalRadio_find_i(self, id);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        list->f->rm(list, i);
    }
}
__noused static inline __ASignalLink* __ASignalRadio_find(const __ASignalRadio* self, int64_t id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    uint32_t i = __ASignalRadio_find_i(self, id);
    auto list = &self->linkpList;
    if(i != 0xffffffff){
        auto pp = list->f->at(list, i);
        if(pp != nullptr) return *pp;
    }

    return nullptr;
}
static inline bool __ASignalRadio_exist(const __ASignalRadio* self, int64_t id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }
    if(__a_unlikely(id <= 0)){
        aExcSet(AEXC_outdomain);
        return false;
    }

    uint32_t i = __ASignalRadio_find_i(self, id);
    if(i != 0xffffffff){
        return true;
    }

    return false;
}
static inline bool __ASignalRadio_empty(const __ASignalRadio* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return false;
    }

    auto list = &self->linkpList;
    return list->f->empty(list);
}

AHash_Define(const_ptr_t,__ASignalRadio);
AHash_Generate(const_ptr_t,__ASignalRadio);
A_TYPE_REGISTER(AHash(const_ptr_t,__ASignalRadio));



/* 信号系统 */
typedef struct{
#if __SIZEOF_POINTER__ == 8
    atomic_llong                        count;
#else
    atomic_long                         count;
#endif /* 64 or 32 */
    AList(__ASignalLink)                linkPool;
    AHash(int64_t,__ASignalTower)       idMap;
    AHash(const_ptr_t,__ASignalRadio)   adMap;
}__ASignalSystem;
A_TYPE_REGISTER(__ASignalSystem);
__noused __visibility(hidden) void A_OBJ_INIT(__ASignalSystem)(__ASignalSystem* self){
    aExcClean();
    atomic_store_explicit(&self->count, 1, memory_order_relaxed);
    self->linkPool = A_INIT(AList(__ASignalLink));
    if(aExcOccur()) return;
    self->idMap = A_INIT(AHash(int64_t, __ASignalTower));
    if(aExcOccur()) return;
    self->adMap = A_INIT(AHash(const_ptr_t,__ASignalRadio));
    if(aExcOccur()) return;
}
__noused __visibility(hidden) void A_OBJ_DEST(__ASignalSystem)(__ASignalSystem* self){
    atomic_store_explicit(&self->count, 0, memory_order_relaxed);
    A_DEST(AList(__ASignalLink), self->linkPool);
    A_DEST(AHash(int64_t,__ASignalTower), self->idMap);
    A_DEST(AHash(const_ptr_t,__ASignalRadio), self->adMap);
}
__noused __visibility(hidden) void A_OBJ_COPY(__ASignalSystem)(__ASignalSystem* self, __noused const __ASignalSystem* that){
    memset(self, 0, sizeof(__ASignalSystem));
}
__noused __visibility(hidden) int A_OBJ_CMPD(__ASignalSystem)(const __ASignalSystem* self, const __ASignalSystem* that){
    int ret = A_CMPD(int64_t, (int64_t)self->count, (int64_t)that->count);
    if(ret == 0) ret = A_CMPD(AList(__ASignalLink), self->linkPool, that->linkPool);
    if(ret == 0) ret = A_CMPD(AHash(int64_t,__ASignalTower), self->idMap, that->idMap);
    if(ret == 0) ret = A_CMPD(AHash(const_ptr_t,__ASignalRadio), self->adMap, that->adMap);
    return ret;
}

static inline int64_t __ASignalSystem_alloc(__ASignalSystem* self){
    if(__a_unlikely(self == nullptr)){
        return -1;
    }
    return atomic_fetch_add(&self->count, 1);
}
static inline __ASignalRadio* __ASignalSystem_find_forad(const __ASignalSystem* self, const void* addressee){
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
static inline __ASignalTower* __ASignalSystem_find_forid(const __ASignalSystem* self, int64_t id){
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
static inline __ASignalLink* __ASignalSystem_find(const __ASignalSystem* self, int64_t id, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }

    if(__a_unlikely(id <= 0 || id >= self->count || addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    auto tower = __ASignalSystem_find_forid(self, id);
    if(tower == nullptr){
        return nullptr;
    }
    return __ASignalTower_find(tower, addressee);
}
static inline void __ASignalSystem_add(__ASignalSystem* self, int64_t id, const void* addressee, void(*call)(const ASignal*, void*)){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(id <= 0 || id >= self->count || addressee == nullptr || call == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    __ASignalLink* link = __ASignalSystem_find(self, id, addressee);
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
        RAII(__ASignalTower) x = A_INIT(__ASignalTower);
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
        RAII(__ASignalRadio) x = A_INIT(__ASignalRadio);
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
        pool->f->pushFront(pool, (__ASignalLink){ .id = id, .addressee = addressee, .call = call });
        if(aExcOccur()){
            return;
        }
        link = pool->f->at(pool, 0);
        if(link == nullptr){
            return;
        }
    }
    if(!__ASignalTower_exist(tower, addressee)){
        __ASignalTower_add(tower, link);
        if(aExcOccur()){
            pool->f->rm_p(pool, link);
            return;
        }
    }
    if(!__ASignalRadio_exist(radio, id)){
        __ASignalRadio_add(radio, link);
        if(aExcOccur()){
            __ASignalTower_rm(tower, addressee);
            pool->f->rm_p(pool, link);
            return;
        }
    }
}
static inline void __ASignalSystem_rm_link(__ASignalSystem* self, __ASignalLink* link){
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

    auto tower = __ASignalSystem_find_forid(self, link->id);
    if(tower != nullptr){
        __ASignalTower_rm(tower, link->addressee);
        if(__ASignalTower_empty(tower)){
            auto map = &self->idMap;
            map->f->rm(map, link->id);
        }
    }
    auto radio = __ASignalSystem_find_forad(self, link->addressee);
    if(radio != nullptr){
        __ASignalRadio_rm(radio, link->id);
        if(__ASignalRadio_empty(radio)){
            auto map = &self->adMap;
            map->f->rm(map, link->addressee);
        }
    }

    auto pool = &self->linkPool;
    pool->f->rm_p(pool, link);
}
static inline void __ASignalSystem_rm_forid(__ASignalSystem* self, int64_t id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(id <= 0 || id >= self->count)){
        aExcSet(AEXC_outdomain);
        return;
    }

    auto tower_p = __ASignalSystem_find_forid(self, id);
    if(tower_p != nullptr){
        RAII(__ASignalTower) tower = A_COPY(__ASignalTower, *tower_p);
        forEach(it, tower.linkpList){
            auto link = it.p != nullptr ? *it.p : nullptr;
            if(link != nullptr){
                __ASignalSystem_rm_link(self, link);
            }
        }
    }
}
static inline void __ASignalSystem_rm_forad(__ASignalSystem* self, const void* addressee){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(addressee == nullptr)){
        aExcSet(AEXC_outdomain);
        return;
    }

    auto radio_p = __ASignalSystem_find_forad(self, addressee);
    if(radio_p != nullptr){
        RAII(__ASignalRadio) radio = A_COPY(__ASignalRadio, *radio_p);
        forEach(it, radio.linkpList){
            auto link = it.p != nullptr ? *it.p : nullptr;
            if(link != nullptr){
                __ASignalSystem_rm_link(self, link);
            }
        }
    }
}
static inline __ASignalTower __ASignalSystem_get_tran_list(__ASignalSystem* self, const ASignal* signal){
    __ASignalTower tower = {};

    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return tower;
    }

    if(__a_unlikely(signal == nullptr || signal->id <= 0 || signal->id >= self->count)){
        aExcSet(AEXC_outdomain);
        return tower;
    }

    auto id = signal->id;
    auto tower_p = __ASignalSystem_find_forid(self, id);
    if(tower_p == nullptr){
        aExcSet(AEXC_outdomain);
        return tower;
    }

    aExcClean();
    tower = A_COPY(__ASignalTower, *tower_p);
    if(aExcOccur()){
        return tower;
    }

    return tower;
}
static void __ASignalSystem_set_call(__ASignalSystem* self, __ASignalTower tower){
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
static void __ASignalSystem_clean_call(__ASignalSystem* self, __ASignalTower tower){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    forEach(it, tower.linkpList){
        if(it.p != nullptr && *it.p != nullptr){
            auto link = *it.p;
            link->call_num--;
            if(link->dis_request){
                __ASignalSystem_rm_link(self, link);
            }
        }
    }
}



static AMtxRW a_call_lock;
static AMtxRW a_global_lock;

static bool a_system_flag = false;

/* 防止同线程递归加锁 */
static thread_local int  a_call_num = 0;

static __ASignalSystem a_signal_system;

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

    a_signal_system = A_INIT(__ASignalSystem);

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
        A_DEST(__ASignalSystem, a_signal_system);
        A_DEST(AMtxRW, a_global_lock);
        A_DEST(AMtxRW, a_call_lock);
        a_system_flag = false;
    }
}



int64_t a_signal_alloc(void){
    aExcClean();

    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return -1;
    }

    return __ASignalSystem_alloc(&a_signal_system);
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

    RAII(__ASignalTower) tower = {};

    {
        RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
        if(aExcOccur()){
            return ret;
        }

        tower = __ASignalSystem_get_tran_list(&a_signal_system, signal);
        if(aExcOccur()){
            return ret;
        }

        __ASignalSystem_set_call(&a_signal_system, tower);
        if(aExcOccur()){
            return ret;
        }
    }

    a_call_num++;
    {
        aExcClean();
        if(__ASignalTower_call(&tower, signal, collector) == AEXC_response_exc){
            ret = AEXC_response_exc;
        }
        aExcClean();
    }

    do{
        RAII(AAutoKey) autokey_g = AMtxRW_wlock(&a_global_lock);
        if(aExcOccur()){
            break;
        }

        __ASignalSystem_clean_call(&a_signal_system, tower);
    }while(0);

    a_call_num--;
    return ret;
}

void a_signal_connection(int64_t id, const void* addressee, void(*call)(const ASignal*, void*)){
    if(!a_system_flag){
        aExcSet(AEXC_system_error);
        return;
    }

    aExcClean();
    RAII(AAutoKey) autokey = AMtxRW_wlock(&a_global_lock);
    if(aExcOccur()){
        return;
    }

    __ASignalSystem_add(&a_signal_system, id, (void*)addressee, call);
}

void a_signal_disconnect(int64_t id, const void* addressee){
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

    auto link = __ASignalSystem_find(&a_signal_system, id, addressee);
    if(link == nullptr){
        return;
    }

    __ASignalSystem_rm_link(&a_signal_system, link);
}

void a_signal_disconnect_all(int64_t id){
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

    __ASignalSystem_rm_forid(&a_signal_system, id);
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

    __ASignalSystem_rm_forad(&a_signal_system, addressee);
}

