/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __afile_h__
#define __afile_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "atext.h"
#include "aclass.h"
#include <stdio.h>

/* file基类 */
AClass_Inherit(AFile);
AClass_Struct(AFile,
    FILE*       fp;
    AText       name;
    uint64_t    size;
);
AClass_Function(AFile);
AClass_Generate(AFile);
__weak __visibility(protected) void A_OBJ_INIT(AFile)(AFile* self){
    self->name = A_INIT(AText);
}
__weak __visibility(protected) void A_OBJ_DEST(AFile)(AFile* self){
    if(self->fp != nullptr) fclose(self->fp);
    A_DEST(AText, self->name);
}
__weak __visibility(protected) void A_OBJ_COPY(AFile)(AFile* self, const AFile* that){
    self->name = A_COPY(AText, that->name);
    if(AText_empty(&self->name)) self->name = AText_new("(null)");
}
__weak __visibility(protected) int A_OBJ_CMPD(AFile)(const AFile* self, const AFile* that){
    return A_CMPD(AText, self->name, that->name);
}
A_CLASS_REGISTER(AFile);
void __AFile_open(AFile* self, const char* mode);



/* file读取器 */
AClass_Inherit(ARFile, AFile);
AClass_Struct(ARFile,
    const void* mem;    //以文件的形式处理mem
    uint64_t offset;
);
AClass_Function(ARFile,
    uint64_t(*size)(ARFile* self); //剩余未读的大小
    uint32_t(*read)(ARFile* self, uint32_t size, void* target);
);
uint64_t ARFile_size(ARFile* self);
uint32_t ARFile_read(ARFile* self, uint32_t size, void* target);
AClass_Generate(ARFile, ARFile_size, ARFile_read);
__weak __visibility(protected) void A_OBJ_COPY(ARFile)(ARFile* self, const ARFile* that){
    self->mem = that->mem;
    if(self->mem == nullptr){
        __AFile_open((AFile*)self, "r");
    }else{
        ((AFile*)self)->size = ((AFile*)that)->size;
    }
}
A_CLASS_REGISTER(ARFile);



/* file写入器 */
AClass_Inherit(AWFile, AFile);
AClass_Struct(AWFile,
    uint64_t addsize;
);
AClass_Function(AWFile,
    uint64_t(*size)(AWFile* self);  //已写入的大小
    uint32_t(*write)(AWFile* self, uint32_t size, void* target);
);
uint64_t AWFile_size(AWFile* self);
uint32_t AWFile_write(AWFile* self, uint32_t size, void* target);
AClass_Generate(AWFile, AWFile_size, AWFile_write);
__weak __visibility(protected) void A_OBJ_COPY(AWFile)(AWFile* self, __noused const AWFile* that){
    __AFile_open((AFile*)self, "w");
}
A_CLASS_REGISTER(AWFile);



/* file追加器 */
AClass_Inherit(APFile, AFile);
AClass_Struct(APFile,
    uint64_t addsize;
);
AClass_Function(APFile,
    uint64_t(*size)(APFile* self); //文件大小+追加的大小
    uint32_t(*append)(APFile* self, uint32_t size, void* target);
);
uint64_t APFile_size(APFile* self);
uint32_t APFile_append(APFile* self, uint32_t size, void* target);
AClass_Generate(APFile, APFile_size, APFile_append);
__weak __visibility(protected) void A_OBJ_COPY(APFile)(APFile* self, __noused const APFile* that){
    __AFile_open((AFile*)self, "a");
}
A_CLASS_REGISTER(APFile);



ARFile aFileOpen(const char* name);
AWFile aFileCreate(const char* name);
APFile aFileAppend(const char* name);
ARFile aMemoryOpen(const void* mem, uint64_t size);



#if defined(__C_POSIX__)
#include <fcntl.h>          // open
#include <unistd.h>         // read, write, close
#include <sys/ioctl.h>      // ioctl

AClass_Inherit(ADev);
AClass_Struct(ADev,
    AText   name;           //设备名
    int32_t fd;             //设备描述符
    bool    noblock;        //是否非阻塞
    bool    stat;           //设备状态
);
AClass_Function(ADev,
    int32_t (*ioctl)(ADev* self, int32_t cmd, void* buf);
    uint32_t(*read) (ADev* self, uint32_t size, void* source);
    uint32_t(*write)(ADev* self, uint32_t size, void* target);
);
int32_t  ADev_ioctl(ADev* self, int32_t cmd, void* buf);
uint32_t ADev_read (ADev* self, uint32_t size, void* source);
uint32_t ADev_write(ADev* self, uint32_t size, void* target);
AClass_Generate(ADev, ADev_ioctl, ADev_read, ADev_write);

__weak __visibility(protected) void A_OBJ_INIT(ADev)(ADev* self){
    self->name = A_INIT(AText);
    self->fd = 0;
}
__weak __visibility(protected) void A_OBJ_DEST(ADev)(ADev* self){
    close(self->fd);
    A_DEST(AText, self->name);
}
__weak __visibility(protected) void A_OBJ_COPY(ADev)(ADev* self, const ADev* that){
    self->name = A_COPY(AText, that->name);
    self->noblock = that->noblock;
    if(self->noblock){
        self->fd = open(self->name.s, O_RDWR | O_NONBLOCK);
    }else{
        self->fd = open(self->name.s, O_RDWR);
    }
    if(self->fd < 0){
        aExcSet(AEXC_system_error);
        return;
    }
    self->stat = true;
}
__weak __visibility(protected) int  A_OBJ_CMPD(ADev)(const ADev* self, const ADev* that){
    return A_CMPD(AText, self->name, that->name);
}


A_CLASS_REGISTER(ADev);

ADev aDevOpen(const char* name);    //阻塞模式打开
ADev aDevOpen_nb(const char* name); //非阻塞模式打开

#endif /* posix */



#if defined(__C_WINDOWS__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>

typedef struct{
    void*    in;
    uint32_t in_size;
    void*    out;
    uint32_t out_size;
    uint32_t bytes;
}ADevIoctl;

AClass_Inherit(ADev);
AClass_Struct(ADev,
    AText   name;           //设备名
    HANDLE  fd;             //设备句柄
    bool    noblock;        //是否非阻塞
    bool    stat;           //设备状态
);
AClass_Function(ADev,
    int32_t (*ioctl)(ADev* self, int32_t cmd, void* buf);
    uint32_t(*read) (ADev* self, uint32_t size, void* source);
    uint32_t(*write)(ADev* self, uint32_t size, void* target);
);
int32_t  ADev_ioctl(ADev* self, int32_t cmd, void* buf);
uint32_t ADev_read (ADev* self, uint32_t size, void* source);
uint32_t ADev_write(ADev* self, uint32_t size, void* target);
AClass_Generate(ADev, ADev_ioctl, ADev_read, ADev_write);

__weak __visibility(protected) void A_OBJ_INIT(ADev)(ADev* self){
    self->name = A_INIT(AText);
    self->fd = INVALID_HANDLE_VALUE;
}
__weak __visibility(protected) void A_OBJ_DEST(ADev)(ADev* self){
    if(self->fd != INVALID_HANDLE_VALUE) CloseHandle(self->fd);
    A_DEST(AText, self->name);
}
__weak __visibility(protected) void A_OBJ_COPY(ADev)(ADev* self, const ADev* that){
    self->name = A_COPY(AText, that->name);
    self->noblock = that->noblock;

    DWORD flags = FILE_ATTRIBUTE_NORMAL;
    if(self->noblock) flags |= FILE_FLAG_OVERLAPPED;

    self->fd = CreateFileA(self->name.s, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, flags, nullptr);
    if(self->fd == INVALID_HANDLE_VALUE){
        aExcSet(AEXC_system_error);
        return;
    }
    self->stat = true;
}
__weak __visibility(protected) int  A_OBJ_CMPD(ADev)(const ADev* self, const ADev* that){
    return A_CMPD(AText, self->name, that->name);
}

A_CLASS_REGISTER(ADev);

ADev aDevOpen(const char* name);    //阻塞模式打开
ADev aDevOpen_nb(const char* name); //非阻塞模式打开

#endif /* windows */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__afile_h__*/
