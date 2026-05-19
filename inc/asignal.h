/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __asignal_h__
#define __asignal_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "aline.h"
#include "aclass.h"

typedef int32_t Aint;
__unused static inline void A_OBJ_INIT(Aint)(Aint* self){ *self = 0; }
__unused static inline void A_OBJ_DEST(Aint)(Aint* self){ *self = 0; }
__unused static inline void A_OBJ_COPY(Aint)(Aint* self, const Aint* that){ *self = *that; }
__unused static inline int  A_OBJ_CMPD(Aint)(const Aint* self, const Aint* that){ return *self==*that ? 0:(*self>*that? 1 : -1); }
A_TYPE_REGISTER(Aint);



/* 信号基类 */
/* 用户需要继承这个类实现自己的信号 */
/* 基类不提供信号校验，如有需要需在子类中自行实现 */
AClass_Inherit(ASignal);
AClass_Struct(ASignal,
    Aint                id;         //信号id,由信号系统分配
    Aint                value;      //信号值
    const void*         sender;     //发送者
    const char*         signal_name;//信号名
);
AClass_Function(ASignal);
AClass_Generate(ASignal);
__unused static inline int A_OBJ_CMPD(ASignal)(const ASignal* self, const ASignal* that){
    int ret = A_CMPD(Aint, self->id, that->id);
    if(ret == 0) ret = A_CMPD(Aint, self->value, that->value);
    if(ret == 0) ret = A_CMPD(cptr_t, (void*)self->sender, (void*)that->sender);
    return ret;
}
A_CLASS_REGISTER(ASignal);



/* 异常收集器 */
typedef struct{
    const void* addressee;
    AEXC_t      exc_value;
}AExcEnd;
A_TYPE_REGISTER(AExcEnd);

ALine_Define(AExcEnd);
ALine_Generate(AExcEnd);
A_TYPE_REGISTER(ALine(AExcEnd));

AClass_Inherit(AExcCollector);
AClass_Struct(AExcCollector,
    Aint            id;
    AEXC_t          exc;            //收集器异常标志
    const void*     sender;         //发送者
    const char*     signal_name;    //信号名
    ALine(AExcEnd)  list;           //异常列表
);
AClass_Function(AExcCollector,
    AExcEnd (*const pop)(AExcCollector* collector);
    bool    (*const empty)(const AExcCollector* collector);
    uint32_t(*const getNumber)(const AExcCollector* collector);
);
AExcEnd AExcCollector_pop(AExcCollector* collector);
bool AExcCollector_empty(const AExcCollector* collector);
uint32_t AExcCollector_getNumber(const AExcCollector* collector);
AClass_Generate(AExcCollector,
    AExcCollector_pop,
    AExcCollector_empty,
    AExcCollector_getNumber,
);
__unused static inline void A_OBJ_INIT(AExcCollector)(AExcCollector* self){
    self->list = A_INIT(ALine(AExcEnd));
}
__unused static inline void A_OBJ_DEST(AExcCollector)(AExcCollector* self){
    A_DEST(ALine(AExcEnd), self->list);
}
__unused static inline void A_OBJ_COPY(AExcCollector)(AExcCollector* self, const AExcCollector* that){
    *self = *that;
    self->list = A_COPY(ALine(AExcEnd), that->list);
}
__unused static inline int  A_OBJ_CMPD(AExcCollector)(const AExcCollector* self, const AExcCollector* that){
    int ret = A_CMPD(Aint, self->id, that->id);
    if(ret == 0) ret = A_CMPD(int, (int)self->exc, (int)that->exc);
    if(ret == 0) ret = A_CMPD(ALine(AExcEnd), self->list, that->list);
    return ret;
}
A_CLASS_REGISTER(AExcCollector);



/* 信号靶函数 */
//void (*call)(const ASignal* signal, void* addressee);

/* 信号系统API */
/* 分配信号id */
Aint a_signal_alloc(void);

/* 发送信号 */
/* 信号发出后后调用对应的靶函数 */
/* collector用于收集靶函数抛出的异常 */
/* 返回值表示靶函数是否抛出了异常，其取值仅为0或AEXC_response_exc */
int __a_signal_transmit(const ASignal* signal, AExcCollector* collector);

#define _a_signal_transmit(signal, collector, ...)({                                \
    auto __a_si = (signal); auto __a_co = (collector);                              \
    __a_type_assert(ASignal, *__a_si);                                              \
    __a_type_assert(AExcCollector,*__a_co);                                         \
    __a_signal_transmit(__a_si, __a_co);                                            \
})                                                                                  \

#define a_signal_transmit(signal, ...)({                                            \
    auto __a_si_x = (signal);                                                       \
    static_assert(__aNarg_n(0, ##__VA_ARGS__) <= 1,                                 \
            "a_signal_transmit(const ASignal* signal, AExcCollector* collector)");  \
    _a_signal_transmit(__a_si_x, ##__VA_ARGS__, (AExcCollector*)nullptr);           \
})                                                                                  \



/* 连接信号与靶 */
/* 同一个(id, addressee)重复连接时会覆盖原有回调 */
void a_signal_connection(Aint id, const void* addressee, void(*call)(const ASignal*, void*));

/* 断开连接 */
void a_signal_disconnect(Aint id, const void* addressee);
__unused static inline void a_target_disconnect(const void* addressee, Aint id){
    a_signal_disconnect(id, addressee);
}


/* 断开全部连接 */
void a_signal_disconnect_all(Aint id);
void a_target_disconnect_all(const void* addressee);



/*
 *             使用约定
 * 1 已连接信号的对象在析构前必须断开连接
 * 2 不在靶函数中等待其他线程的信号操作，否则可能会死锁
 * 3 可以在靶函数中断开连接，但尽量不要析构目标对象，若必须析构，则析构后不可再使用
 *
 * 只要满足上面的约定即可安全使用对象系统
 */



/* 接收者基类 */
/* 继承此类后析构时自动disconnect */
/* 即使不继承此类也可通过api disconnect */
AClass_Inherit(AReceEnd);
AClass_Struct(AReceEnd,
);
AClass_Function(AReceEnd,
    void(*const connection)(const AReceEnd* self, Aint id, void(*call)(const ASignal* signal, void* addressee));
    void(*const disconnect)(const AReceEnd* self, Aint id);
);

__unused static inline void AReceEnd_connection(const AReceEnd* self, Aint id, void(*call)(const ASignal* signal, void* addressee)){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    a_signal_connection(id, self, call);
}
__unused static inline void AReceEnd_disconnect(const AReceEnd* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    a_target_disconnect(self, id);
}
AClass_Generate(AReceEnd, AReceEnd_connection, AReceEnd_disconnect);

__unused static inline void A_OBJ_DEST(AReceEnd)(AReceEnd* self){
    a_target_disconnect_all(self);
}
A_CLASS_REGISTER(AReceEnd);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__asignal_h__*/

