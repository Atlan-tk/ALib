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
#include "aclass.h"
#include "astring.h"

#include <stdio.h>
#include <stdatomic.h>

/* 创建目录 */
/* af_mkdir("/tmp/test_dir"); 相当于mkdir -p*/
void af_mkdir(const char* name);

/* 删除文件或目录 */
/* af_rm("/tmp/test.txt") */
void af_rm(const char* name);
/* af_rm_r("/tmp/test_dir/") */
void af_rm_r(const char* name);

/* 复制文件或目录 */
/* af_cp("/tmp/test.txt", "/tmp/test1.txt") */
void af_cp(const char* name, const char* target);
/* af_cp_r("/tmp/test_dir", "/tmp/test_dir1") */
void af_cp_r(const char* name, const char* target);

/* 移动文件或目录 */
/* af_mv("/tmp/test.txt", "/tmp/test1.txt") */
void af_mv(const char* name, const char* new_name);

/* 创建空文件 */
/* 若已存在则直接返回 */
/* af_touch("/tmp/test.txt") */
void af_touch(const char* name);

/* 修改当前用户文件权限 */ /* p=0|1|2|4 x/w/r */
/* af_chmod("/tmp/test.txt", 0644) */
void af_chmod(const char* name, int p);
/* af_chmod_r("/tmp/test_dir", 0755); chmod -R */
void af_chmod_r(const char* name, int p);

/* 路径存在 */
bool af_path_exist(const char* name);
/* 目标是文件 *//* 包含设备 */
bool af_isfile(const char* name);
/* 目标是目录 */
bool af_isdir(const char* name);
/* 目标是设备 */
bool af_isdev(const char* name);

/* 提取上层目录 */
/* 输入根目录返回根目录 *//* 末尾不带'/' */
/* af_dir_extract("/tmp/test.txt"); return "/tmp" */
AStr af_dir_extract(const char* name);

/* 提取文件/目录名 */
/* 输入根目录返回根目录 *//* 末尾不带'/' */
/* af_dir_extract("/tmp/test.txt"); return "test.txt" */
/* af_dir_extract("/tmp/test_dir"); return "test_dir" */
AStr af_name_extract(const char* name);

/* 获取绝对路径 *//* 末尾不带'/' */
AStr af_path_absolute(const char* name);

ALine_Define(AStr);
ALine_Generate(AStr);
A_TYPE_REGISTER(ALine(AStr));

/* 获取文件/目录列表 */
/* af_ls("/tmp/test_dir/"); ls -A */
ALine(AStr) af_ls(const char* dir);

/* file info */
typedef int64_t stat_time_t;
typedef struct{
    uint64_t    ast_dev;     /* 设备 ID  */
    uint64_t    ast_ino;     /* 文件索引 */
    uint32_t    ast_mode;    /* 文件类型 + 权限位 */
    uint32_t    ast_nlink;   /* 硬链接数 */
    uint32_t    ast_uid;     /* 所有者用户 ID */
    uint32_t    ast_gid;     /* 所有者组 ID */
    uint64_t    ast_rdev;    /* 设备 ID */
    uint64_t    ast_size;    /* 文件大小（字节） */
    stat_time_t ast_atime;   /* 最后访问时间 */
    stat_time_t ast_mtime;   /* 最后修改时间 */
    stat_time_t ast_ctime;   /* 最后状态更改时间（近似为写入时间） */
}AFileInfo;
A_TYPE_REGISTER(AFileInfo);
AFileInfo af_get_info(const char* name);



#if defined(__C_POSIX__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>      // ioctl
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

typedef int Afd;
__noused static void A_OBJ_INIT(Afd)(Afd* self){ *self = -1; }
__noused static void A_OBJ_DEST(Afd)(Afd* self){ *self = -1; }
__noused static void A_OBJ_COPY(Afd)(Afd* self, const Afd* that){ *self = *that; }
__noused static int  A_OBJ_CMPD(Afd)(const Afd* self, const Afd* that){ return A_CMPD(int, *self, *that); }
A_TYPE_REGISTER(Afd);

static inline bool Afd_exist(Afd fd){
    return fd < 0 ? false : true;
}

static inline int a_close(Afd fd){
    return close(fd);
}

ALine_Define(Afd);
ALine_Generate(Afd);
A_TYPE_REGISTER(ALine(Afd));

typedef pid_t Apid;
static inline Apid a_get_pid(void){ return getpid(); }

typedef struct flock AFileLock;
__noused static void A_OBJ_INIT(AFileLock)(AFileLock* self){
    *self = (AFileLock){
        .l_type   = F_UNLCK,
        .l_whence = SEEK_SET,
        .l_start  = 0,
        .l_len    = 0,          // 整个文件
        .l_pid    = 0,          // 不用设置
    };
}
__noused static void A_OBJ_COPY(AFileLock)(__noused AFileLock* self, __noused const AFileLock* that){
    //不可复制
    aExcSet(AEXC_init_failed);
}
A_TYPE_REGISTER(AFileLock);
static inline int AFileLock_uplock_read(AFileLock* self, Afd fd){
    self->l_type = F_RDLCK;
    int ret = fcntl(fd, F_SETLKW, self);
    if(ret == -1){
        aExcSet(AEXC_system_error);
    }
    return ret;
}

static inline int AFileLock_uplock_write(AFileLock* self, Afd fd){
    self->l_type = F_WRLCK;
    int ret = fcntl(fd, F_SETLKW, self);
    if(ret == -1){
        aExcSet(AEXC_system_error);
    }
    return ret;
}

static inline int AFileLock_unlock(AFileLock* self, Afd fd){
    self->l_type = F_UNLCK;
    int ret = fcntl(fd, F_SETLKW, self);
    if(ret == -1){
        aExcSet(AEXC_system_error);
    }
    return ret;
}

#endif /* posix */



typedef struct{
    AStr            name;       //绝对路径
    AFileLock       filelock;   //文件锁
    AMtx            fl_lock;    //文件锁保护锁
    AMtxRW          lock;       //进程内读写锁
    ALine(Afd)      fd_list;    //废弃的fd, 析构时统一释放
    Afd             fd;         //文件描述符
    Apid            pid;        //pid
    thrd_t          tid;        //tid
    bool            fl_stat;    //文件锁状态
    int32_t         mod;        //模式 & 属性
    uint32_t        type;       //0.file, 1.device, 2+.socket
    uint32_t        num;        //连接数量
    uint32_t        rnum;       //读者数
}AFileNode;
enum{
    __afmod_read = 0x0001,
    __afmod_write = 0x0002,
};
enum{
    __afmod_creat = 0x0004,
    __afmod_appent = 0x0008,
    __afmod_truncate = 0x0010,
    __afmod_noblock = 0x0200,
    __afmod_exclusive = 0x0400,
};
enum{
    __aftype_file = 1,
    __aftype_device,
    __aftype_socket,    //持不支持域名,需要自行转换
                        //支持tcp|udp|raw|unix
};

__noused static inline void A_OBJ_INIT(AFileNode)(AFileNode* self){
    memset(self, 0, sizeof(AFileNode));

    self->fd = A_INIT(Afd);
    self->pid = a_get_pid();
    self->tid = thrd_current();
    self->name = A_INIT(AStr);
    self->lock = A_INIT(AMtxRW);
    self->fl_lock = A_INIT(AMtx);
    self->filelock= A_INIT(AFileLock);
    self->fd_list = A_INIT(ALine(Afd));
}
__noused static inline void A_OBJ_DEST(AFileNode)(AFileNode* self){
    if(self->fl_stat && (self->mod & __afmod_exclusive)){
        AFileLock_unlock(&self->filelock, self->fd);
        AMtxRW_unlock_write(&self->lock);
        self->fl_stat = false;
    }
    forEach(it, self->fd_list){
        auto fd = *it.p;
        if(Afd_exist(fd)) a_close(fd);
    }
    if(Afd_exist(self->fd)) a_close(self->fd);
    A_DEST(AMtx, self->fl_lock);
    A_DEST(ALine(Afd), self->fd_list);
    A_DEST(AStr, self->name);
    A_DEST(AMtxRW, self->lock);
}
__noused static inline void A_OBJ_COPY(AFileNode)(__noused AFileNode* self, __noused const AFileNode* that){
    //不可复制
    aExcSet(AEXC_init_failed);
}
__noused static inline int A_OBJ_CMPD(AFileNode)(const AFileNode* self, const AFileNode* that){
    return A_CMPD(AStr, self->name, that->name);
}
A_TYPE_REGISTER(AFileNode);
/*
 * 文件模式(r|w|rw)完全互斥，即以不同模式重复打开同一文件时会直接失败
 * 独占仅在(w|rw|)模式下可用，处于独占模式时直到文件关闭，期间始终持有写锁，其他任何打开操作都会阻塞
 * r模式下自动使用共享方法，即持有读锁
 */



AShPtr_Define(AFileNode);
AShPtr_Generate(AFileNode);
A_TYPE_REGISTER(AShPtr(AFileNode));



/* file基类 */
AClass_Inherit(AFile);
AClass_Struct(AFile,
    Afd                 fd;
    AStr                name;
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
void AFile_open(AFile* self, int type, AStr name, int mod);
/*
 *  AFile_open(&file, __aftype_file, "/tmp/test.txt", __afmod_read);
 *  AFile_open(&file, __aftype_file, "/tmp/test.txt", __afmod_write | __afmod_exclusive);
 *  AFile_open(&file, __aftype_file, "/tmp/test.txt", __afmod_write | __afmod_appent | __afmod_exclusive);
 *  AFile_open(&file, __aftype_device, "/dev/test", __afmod_read | __afmod_noblock);
 *  AFile_open(&file, __aftype_device, "/dev/test", __afmod_write | __afmod_exclusive);
 *  AFile_open(&file, __aftype_device, "/dev/test", __afmod_read | __afmod_write | __afmod_noblock | __afmod_exclusive);
 *  AFile_open(&file, __aftype_socket, "tcp|server|8290|0.0.0.0", __afmod_read | __afmod_write);
 *  AFile_open(&file, __aftype_socket, "tcp|client|8290|www.name.com", __afmod_read | __afmod_write);
 *  AFile_open(&file, __aftype_socket, "udp|server|8290|0.0.0.0", __afmod_read | __afmod_write);
 *  AFile_open(&file, __aftype_socket, "raw|server|8290|192.168.0.1", __afmod_read | __afmod_write);
 *  AFile_open(&file, __aftype_socket, "unix|server|0|/tmp/test.txt", __afmod_read | __afmod_write);
 */
void AFile_register(AFile* self, Afd fd, int type, AStr name, int mod);
/*
 *  注册fd并构造file
 *  AFile_register(&file, fd, __aftype_file, "/tmp/test.txt", __afmod_w | __afmod_exclusive);
 */
static inline int AFile_getmod(const AFile* self){
    if(self != nullptr && self->node.p != nullptr)
        return self->node.p->mod;
    return -1;
}
static inline int AFile_gettype(const AFile* self){
    if(self != nullptr && self->node.p != nullptr)
        return self->node.p->type;
    return -1;
}

__noused static inline void A_OBJ_INIT(AFile)(AFile* self){
    self->node = A_INIT(AShPtr(AFileNode));
    self->name = A_INIT(AStr);
    self->fd = -1;
}
__noused static inline void A_OBJ_DEST(AFile)(AFile* self){
    AFile_close(self);
    aExcClean();
}
__noused static inline void A_OBJ_COPY(AFile)(AFile* self, const AFile* that){
    int mod = AFile_getmod(that);
    int type = AFile_gettype(that);
    AFile_open(self, type, self->name, mod);
}
__noused static inline int A_OBJ_CMPD(AFile)(const AFile* self, const AFile* that){
    return A_CMPD(AStr, self->name, that->name);
}
A_CLASS_REGISTER(AFile);




AFile aFileInOpen(const char* name);
AFile aFileOutOpen(const char* name);
AFile aFileEndOpen(const char* name);

AFile aDevInOpen(const char* name, bool noblock);
AFile aDevOutOpen(const char* name, bool exclusive);
AFile aDevInOutOpen(const char* name, bool noblock, bool exclusive);

AFile aSocketTcpServerOpen(const char* ipstr, int port);
AFile aSocketUdpServerOpen(const char* ipstr, int port);
AFile aSocketRawServerOpen(const char* ipstr, int port);
AFile aSocketUnixServerOpen(const char* ipstr);
AFile aSocketTcpClientOpen(const char* ipstr, int port);
AFile aSocketUdpClientOpen(const char* ipstr, int port);
AFile aSocketRawClientOpen(const char* ipstr, int port);
AFile aSocketUnixClientOpen(const char* ipstr);
AFile aSocketTcpAccept(AFile server);
AFile aSocketUnixAccept(AFile server);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__afile_h__*/
