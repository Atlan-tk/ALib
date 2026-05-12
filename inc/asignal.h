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
#include "ahash.h"
#include "aclass.h"

/* 信号基类 */
/* 用户需要继承这个类实现自己的信号 */
AClass_Inherit(ASignal);
AClass_Struct(ASignal,
    int64_t     id;     //信号id,由信号系统分配
    int64_t     value;  //信号值
    const void* sender; //发送者
    const char* name;   //信号名
    const char* code;   //信号校验码
);
AClass_Function(ASignal);
AClass_Generate(ASignal);
A_CLASS_REGISTER(ASignal);

/* 信号的靶函数 */
typedef void (*ASignalTarget)(ASignal* signal, void* addressee);
A_TYPE_REGISTER(ASignalTarget);



/* 信号接收端 */
typedef struct{
    /* 信号接收者 */
    void* addressee;
    /* 信号靶函数 */
    ASignalTarget call;
}ASignalEnd;
A_TYPE_REGISTER(ASignalEnd);

ALine_Define(ASignalEnd);
ALine_Generate(ASignalEnd);
A_TYPE_REGISTER(ALine(ASignalEnd));

ALine_Define(ALine(ASignalEnd));
ALine_Generate(ALine(ASignalEnd));
A_TYPE_REGISTER(ALine(ALine(ASignalEnd)));



/* 信号系统 */
typedef struct{
    int64_t count;
    ALine(ALine(ASignalEnd)) tab;
}ASignalSystem;
static inline void A_OBJ_INIT(ASignalSystem)(ASignalSystem* self){
    self->count = 0;
    self->tab = A_INIT(ALine(ALine(ASignalEnd)));
}
static inline void A_OBJ_DEST(ASignalSystem)(ASignalSystem* self){
    self->count = 0;
    A_DEST(ALine(ALine(ASignalEnd)), self->tab);
}
static inline void A_OBJ_COPY(ASignalSystem)(ASignalSystem* self, const ASignalSystem* that){
    self->count = that->count;
    self->tab = A_COPY(ALine(ALine(ASignalEnd)), that->tab);
}
static inline int  A_OBJ_CMPD(ASignalSystem)(const ASignalSystem* self, const ASignalSystem* that){
    int ret = A_CMPD(int64_t, self->count, that->count);
    return ret != 0 ? ret : A_CMPD(ALine(ALine(ASignalEnd)), self->tab, that->tab);
}
A_TYPE_REGISTER(ASignalSystem)

/* 分配信号id */
int64_t a_signal_system_alloc();
/* 发送信号 */
/* 信号发出后后调用对应的靶函数 */
void a_signal_system_transmit(ASignal* signal);
/* 注册信号靶 */
void a_signal_system_register(int64_t id, void* addressee, ASignalTarget target);
/* 注销信号靶 */
void a_signal_system_unregister(int64_t id, void* addressee);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__asignal_h__*/


