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
static void A_OBJ_INIT(Aint)(Aint* self){ *self = 0; }
static void A_OBJ_DEST(Aint)(Aint* self){ *self = 0; }
static void A_OBJ_COPY(Aint)(Aint* self, const Aint* that){ *self = *that; }
static int  A_OBJ_CMPD(Aint)(const Aint* self, const Aint* that){ return *self==*that ? 0:(*self>*that? 1 : -1); }
A_TYPE_REGISTER(Aint);



typedef struct{
    void*   addressee;
    AEXC_t  exc_value;
}AResponseExc;
A_TYPE_REGISTER(AResponseExc);

ALine_Define(AResponseExc);
ALine_Generate(AResponseExc);
A_TYPE_REGISTER(ALine(AResponseExc));

/* 信号基类 */
/* 用户需要继承这个类实现自己的信号 */
AClass_Inherit(ASignal);
AClass_Struct(ASignal,
    Aint                id;         //信号id,由信号系统分配
    Aint                value;      //信号值
    const void*         sender;     //发送者
    ALine(AResponseExc)*exc_list;   //异常收集, 默认为null,仅当需要时设置
);
AClass_Function(ASignal,
    void(*const setExcList)(ASignal* self);
    void(*const cleanExc)(const ASignal* self);
    AResponseExc(*const popExc)(const ASignal* self);  //push_back
);

void ASignal_setExcList(ASignal* self);
void ASignal_cleanExc(const ASignal* self);
AResponseExc ASignal_popExc(const ASignal* self);
AClass_Generate(ASignal, ASignal_setExcList, ASignal_cleanExc, ASignal_popExc);

static inline void A_OBJ_DEST(ASignal)(ASignal* self){
    if(self->exc_list != nullptr){
        A_DELETE(ALine(AResponseExc), self->exc_list);
        self->exc_list = nullptr;
    }
}
static inline void A_OBJ_COPY(ASignal)(ASignal* self, const ASignal* that){
    *self = *that;
    self->exc_list = nullptr;
}
static inline int A_OBJ_CMPD(ASignal)(const ASignal* self, const ASignal* that){
    int ret = A_CMPD(Aint, self->id, that->id);
    if(ret == 0) ret = A_CMPD(Aint, self->value, that->value);
    if(ret == 0) ret = A_CMPD(cptr_t, (void*)self->sender, (void*)that->sender);
    return ret;
}
A_CLASS_REGISTER(ASignal);



/* 信号靶函数 */
//void (*call)(const ASignal* signal, void* addressee);



/* 信号系统API */
/* 分配信号id */
Aint a_signal_alloc();

/* 发送信号 *//* 信号发出后后调用对应的靶函数 */
void a_signal_transmit(const ASignal* signal);

/* 连接信号与靶 */
void a_signal_connection(Aint id, const void* addressee, void(*call)(const ASignal*, void*));

/* 断开连接 */
void a_signal_disconnect(Aint id, const void* addressee);
static inline void a_target_disconnect(const void* addressee, Aint id){
    a_signal_disconnect(id, addressee);
}


/* 断开全部连接 */
void a_signal_disconnect_all(Aint id);
void a_target_disconnect_all(const void* addressee);



/*
 *             使用约定
 * 1 已连接信号的对象在析构前必须断开连接
 * 2 靶函数中disconnect目标对象会直接失败
 * 3 靶函数中不允许析构任何被连接的对象，否则会造成垂悬指针
 * 4 不在靶函数中等待其他线程的信号操作，否则会死锁
 *
 * 只要满足上面的约定即可安全使用对象系统
 */



/* 发送者基类 */
/* 继承此类后可自动disconnect */
/* 即使不继承此类也可通过api发送信号 */
AClass_Inherit(ATranEnd);
AClass_Struct(ATranEnd,
    Aint id;
);
AClass_Function(ATranEnd,
    Aint(*const getID)(const ATranEnd* self);
    void(*const transmit)(const ASignal* signal);
);

static inline Aint ATranEnd_get_id(const ATranEnd* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr); return -1;
    }
    return self->id;
}
static inline void ATranEnd_transmit(const ASignal* signal){
    a_signal_transmit(signal);
}
AClass_Generate(ATranEnd, ATranEnd_get_id, ATranEnd_transmit);

static void A_OBJ_INIT(ATranEnd)(ATranEnd* self){
    self->id = a_signal_alloc();
}
static void A_OBJ_DEST(ATranEnd)(ATranEnd* self){
    a_signal_disconnect_all(self->id);
    self->id = 0;
}
static void A_OBJ_COPY(ATranEnd)(ATranEnd* self, __unused const ATranEnd* that){
    self->id = a_signal_alloc();
}
static int A_OBJ_CMPD(ATranEnd)(const ATranEnd* self, const ATranEnd* that){
    return A_CMPD(Aint, self->id, that->id);
}
A_CLASS_REGISTER(ATranEnd);

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

static inline void AReceEnd_connection(const AReceEnd* self, Aint id, void(*call)(const ASignal* signal, void* addressee)){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    a_signal_connection(id, self, call);
}
static inline void AReceEnd_disconnect(const AReceEnd* self, Aint id){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    a_target_disconnect(self, id);
}
AClass_Generate(AReceEnd, AReceEnd_connection, AReceEnd_disconnect);

static void A_OBJ_DEST(AReceEnd)(AReceEnd* self){
    a_target_disconnect_all(self);
}
A_CLASS_REGISTER(AReceEnd);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__asignal_h__*/


