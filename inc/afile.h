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
#include "aptr.h"
#include "aline.h"
#include "alock.h"
#include "atext.h"
#include "aclass.h"
#include <stdio.h>
#include <stdatomic.h>

/* 创建目录 */
void af_mkdir(const char* name);
void af_mkdir_p(const char* name);

/* 删除文件或目录 */
void af_rm(const char* name);
void af_rm_r(const char* name);

/* 复制文件或目录 */
void af_cp(const char* name, const char* target);
void af_cp_r(const char* name, const char* target);

/* 移动文件或目录 */
void af_mv(const char* name, const char* new_name);

/* 创建空文件 */
void af_touch(const char* name);

/* 修改当前用户文件权限 */ /* p=0|1|2|4 */
void af_chmod(const char* name, char p);
void af_chmod_r(const char* name, char p);

/* 目标是文件 */
bool af_isfile(const char* name);
/* 目标是目录 */
bool af_isdir(const char* name);
/* 目标是设备 */
bool af_isdev(const char* name);

/* 提取目录 */
/* 若为目录则直接返回 */
/* 若为空则返回当前目录"." */
AText af_dir_extract(const char* name);

/* 获取绝对路径 */
/* 若为空则返回当前目录的绝对路径 */
AText af_path_absolute(const char* name);

/* ls */
ALine_Define(AText);
ALine_Generate(AText);
A_TYPE_REGISTER(ALine(AText));
ALine(AText) af_ls(const char* dir);
ALine(AText) af_ls_a(const char* dir);
ALine(AText) af_ls_A(const char* dir);

/* file info */
typedef int64_t stat_time_t;
typedef struct{
    uint64_t    st_dev;     /* 设备 ID  */
    uint64_t    st_ino;     /* 文件索引 */
    uint32_t    st_mode;    /* 文件类型 + 权限位 */
    uint32_t    st_nlink;   /* 硬链接数 */
    uint32_t    st_uid;     /* 所有者用户 ID */
    uint32_t    st_gid;     /* 所有者组 ID */
    uint64_t    st_rdev;    /* 设备 ID */
    uint64_t    st_size;    /* 文件大小（字节） */
    stat_time_t st_atime;   /* 最后访问时间 */
    stat_time_t st_mtime;   /* 最后修改时间 */
    stat_time_t st_ctime;   /* 最后状态更改时间（近似为写入时间） */
}AFileInfo;
A_TYPE_REGISTER(AFileInfo);
AFileInfo af_get_info(const char* name);



#if defined(__C_POSIX__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>      // ioctl

ALine_Define(int);
ALine_Generate(int);
A_TYPE_REGISTER(ALine(int));

typedef struct{
    AText           name;       //绝对路径
    struct flock    fl;         //文件锁
    AMtx            fl_lock;    //文件锁保护锁
    AMtxRW          lock;       //进程内读写锁
    int             fd;         //文件描述符
    int32_t         mod;        //0只读, 1只写, 2读写, 3追加
    uint32_t        num;        //连接数量
    uint64_t        size;       //文件体积
    pid_t           pid;        //pid
    bool            fl_stat;    //读写者数量
    bool            noblock;    //是否非阻塞
    bool            exclusive;  //是否独占
    uint32_t        rnum;       //读者数
    ALine(int)      fd_list;    //废弃的fd, 析构时统一释放
}AFileNode;
enum{
    __afmod_r = 0,
    __afmod_w = 1,
    __afmod_rw = 2,
    __afmod_aw = 3,
};
__noused static inline void A_OBJ_INIT(AFileNode)(AFileNode* self){
    memset(self, 0, sizeof(AFileNode));

    self->fl = (struct flock){
        .l_type   = F_UNLCK,
        .l_whence = SEEK_SET,
        .l_start  = 0,
        .l_len    = 0,          // 整个文件
        .l_pid    = 0,          // 不用设置
    };
    self->fd = -1;
    self->pid = getpid();
    self->name = A_INIT(AText);
    self->lock = A_INIT(AMtxRW);
    self->fl_lock = A_INIT(AMtx);
    self->fd_list = A_INIT(ALine(int));
}
__noused static inline void A_OBJ_DEST(AFileNode)(AFileNode* self){
    if(self->fl_stat){
        self->fl.l_type = F_UNLCK;
        fcntl(self->fd, F_SETLKW, &self->fl);
        self->fl_stat = false;
    }
    if(self->fd >= 0) close(self->fd);
    forEach(it, self->fd_list){
        auto fd = *it.p;
        if(fd >= 0) close(fd);
    }
    A_DEST(AMtx, self->fl_lock);
    A_DEST(ALine(int), self->fd_list);
    A_DEST(AText, self->name);
    A_DEST(AMtxRW, self->lock);
}
__noused static inline void A_OBJ_COPY(AFileNode)(__noused AFileNode* self, __noused const AFileNode* that){
    //不可复制
    aExcSet(AEXC_init_failed);
}
__noused static inline int A_OBJ_CMPD(AFileNode)(const AFileNode* self, const AFileNode* that){
    return A_CMPD(AText, self->name, that->name);
}
A_TYPE_REGISTER(AFileNode);
/*
 * 文件模式(r|w|rw|aw)完全互斥，即以不同模式重复打开同一文件时会直接失败
 * 独占仅在(w|rw|)模式下可用，处于独占模式时直到文件关闭，期间始终持有写锁，其他任何打开操作都会直接失败
 * r模式下自动使用共享方法，即持有读锁
 * aw模式下自动使用抢占方法，即持有写锁
 */



AShPtr_Define(AFileNode);
AShPtr_Generate(AFileNode);
A_TYPE_REGISTER(AShPtr(AFileNode));



/* file基类 */
AClass_Inherit(AFile);
AClass_Struct(AFile,
    int                 fd;
    AText               name;
    AShPtr(AFileNode)   node;
);
AClass_Function(AFile,
    int32_t (*ioctl)(AFile* self, int32_t cmd, void* buf);
    uint32_t(*read) (AFile* self, size_t size, void* target);
    uint32_t(*write)(AFile* self, size_t size, void* source);
    uint32_t(*read_pos)(AFile* self, uint64_t offset, size_t size, void* target);
    uint32_t(*write_pos)(AFile* self, uint64_t offset, size_t size, void* source);
);
int32_t  AFile_ioctl(AFile* self, int32_t cmd, void* buf);
uint32_t AFile_read (AFile* self, size_t size, void* target);
uint32_t AFile_write(AFile* self, size_t size, void* source);
uint32_t AFile_read_pos(AFile* self, uint64_t offset, size_t size, void* target);
uint32_t AFile_write_pos(AFile* self, uint64_t offset, size_t size, void* source);
AClass_Generate(AFile, AFile_ioctl, AFile_read, AFile_write, AFile_read_pos, AFile_write_pos);

void AFile_close(AFile* self);
AFile AFile_open_copy(AText name);
AFile AFile_open(int mod, bool noblock, bool exclusive, AText name);

__noused static inline void A_OBJ_INIT(AFile)(AFile* self){
    self->node = A_INIT(AShPtr(AFileNode));
    self->name = A_INIT(AText);
    self->fd = -1;
}
__noused static inline void A_OBJ_DEST(AFile)(AFile* self){
    AFile_close(self);
    aExcClean();
}
__noused static inline void A_OBJ_COPY(AFile)(AFile* self, const AFile* that){
    *self = AFile_open_copy(that->name);
}
__noused static inline int A_OBJ_CMPD(AFile)(const AFile* self, const AFile* that){
    return A_CMPD(AText, self->name, that->name);
}
A_CLASS_REGISTER(AFile);

#endif /* posix */



static inline AFile  aFileInOpen(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_outdomain);
        return (AFile){};
    }

    return AFile_open(__afmod_r, false, false, af_path_absolute(name));
}

static inline AFile  aFileEnOpen(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_outdomain);
        return (AFile){};
    }

    return AFile_open(__afmod_aw, false, false, af_path_absolute(name));
}

/* 强制独占 */
static inline AFile aFileOutOpen(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_outdomain);
        return (AFile){};
    }

    return AFile_open(__afmod_w, false, true, af_path_absolute(name));
}



static inline AFile  aDevInOpen(const char* name, bool noblock){
    if(name == nullptr){
        aExcSet(AEXC_outdomain);
        return (AFile){};
    }

    return AFile_open(__afmod_r, noblock, false, af_path_absolute(name));
}

static inline AFile aDevOutOpen(const char* name, bool exclusive){
    if(name == nullptr){
        aExcSet(AEXC_outdomain);
        return (AFile){};
    }

    return AFile_open(__afmod_w, false, exclusive, af_path_absolute(name));
}

static inline AFile aDevInOutOpen(const char* name, bool noblock, bool exclusive){
    if(name == nullptr){
        aExcSet(AEXC_outdomain);
        return (AFile){};
    }

    return AFile_open(__afmod_rw, noblock, exclusive, af_path_absolute(name));
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__afile_h__*/
