/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <afile.h>
#include <ahash.h>
#include <alock.h>
#include <athrd.h>

#if defined(__C_POSIX__)
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*************************************************************************/
AHash_Define(AStr,AShPtr(AFileNode));
AHash_Generate(AStr,AShPtr(AFileNode));
A_TYPE_REGISTER(AHash(AStr,AShPtr(AFileNode)));

static bool         afsflag = false;
static AMtx         afsprwl;
static AMtxRW       afslock;
static AHash(AStr,AShPtr(AFileNode)) afstable;

bool a_fs_start(void){
    aExcClean(); afstable = A_INIT(AHash(AStr,AShPtr(AFileNode))); if(aExcOccur()){
        return false;
    }
    aExcClean(); afslock = A_INIT(AMtxRW);if(aExcOccur()){
        return false;
    }
    aExcClean(); afsprwl = A_INIT(AMtx);if(aExcOccur()){
        return false;
    }
    afsflag = true;
    return true;
}
void a_fs_poweroff(void){
    afsflag = false;
    A_DEST(AHash(AStr,AShPtr(AFileNode)), afstable);
    A_DEST(AMtx, afsprwl);
    A_DEST(AMtxRW, afslock);
}



/*************************************************************************/
/* 若平台未提供pread/pwrite则使用lseek+read/write模拟 */
__noused static ssize_t af_pread_fallback(Afd fd, void* target, size_t size, off_t offset){
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return -1;
    }

    aExcClean(); RAII(AAutoKey) key = AMtx_lock(&afsprwl); if(aExcOccur()){
        return -1;
    }

    ssize_t ret;
    off_t old_offset;

    /* 1. 保存当前文件偏移量 */
    old_offset = lseek(fd, 0, SEEK_CUR);
    if(old_offset == (off_t)-1){
        return -1;
    }

    /* 2. 移动到目标偏移量 */
    if(lseek(fd, offset, SEEK_SET) == (off_t)-1){
        lseek(fd, old_offset, SEEK_SET);
        return -1;
    }

    /* 3. 执行读取 */
    ret = read(fd, target, size);

    /* 4. 恢复原偏移量（即使读取失败也尽量恢复） */
    if(lseek(fd, old_offset, SEEK_SET) == (off_t)-1){
        return -1;
    }

    return ret;
}
__noused static ssize_t af_pwrite_fallback(Afd fd, const void* source, size_t size, off_t offset){
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return -1;
    }

    aExcClean(); RAII(AAutoKey) key = AMtx_lock(&afsprwl); if(aExcOccur()){
        return -1;
    }

    ssize_t ret;
    off_t old_offset;

    /* 1. 保存当前文件偏移量 */
    old_offset = lseek(fd, 0, SEEK_CUR);
    if(old_offset == (off_t)-1){
        return -1;
    }

    /* 2. 移动到目标偏移量 */
    if(lseek(fd, offset, SEEK_SET) == (off_t)-1){
        lseek(fd, old_offset, SEEK_SET);
        return -1;
    }

    /* 3. 执行写入 */
    ret = write(fd, source, size);

    /* 4. 恢复原偏移量（即使写入失败也尽量恢复） */
    if(lseek(fd, old_offset, SEEK_SET) == (off_t)-1){
        return -1;
    }

    return ret;
}

static inline ssize_t af_pread(Afd fd, void* target, size_t size, off_t offset){
#if defined(__GLIBC__) && !defined(__UCLIBC__)
    return pread(fd, target, size, offset);
#else
    return af_pread_fallback(fd, target, size, offset);
#endif
}

static inline ssize_t af_pwrite(Afd fd, const void* source, size_t size, off_t offset){
#if defined(__GLIBC__) && !defined(__UCLIBC__)
    return pwrite(fd, source, size, offset);
#else
    return af_pwrite_fallback(fd, source, size, offset);
#endif
}



/*************************************************************************/
static void __af_mkdir(const char* name);
static void __af_rm(const char* name);
static void __af_rm_r(const char* name);
static void __af_cp(const char* name, const char* target);
static void __af_cp_r(const char* name, const char* target);
static void __af_mv(const char* name, const char* new_name);
static void __af_touch(const char* name);
static void __af_chmod(const char* name, int p);
static void __af_chmod_r(const char* name, int p);
static bool __af_path_exist(const char* name);
static bool __af_isfile(const char* name);
static bool __af_isdir(const char* name);
static bool __af_isdev(const char* name);
static AStr __af_dir_extract(const char* name);
static AStr __af_name_extract(const char* name);
static AStr __af_path_absolute(const char* name);
static AFileInfo __af_get_info(const char* name);
static ALine(AStr) __af_ls(const char* dir);



/*************************************************************************/
typedef struct{
    union{
        struct sockaddr         ip;
        struct sockaddr_un      unp;
        struct sockaddr_in      ipv4;
        struct sockaddr_in6     ipv6;
    };
    int kinds;  //套接字类型
    int len;
}Aaddr;
enum{
    aip_tcp_server = 1,
    aip_tcp_client,
    aip_udp_server,
    aip_udp_client,
    aip_unix_server,
    aip_unix_client,
    aip_raw_server,
    aip_raw_client,
};

static inline Aaddr af_domain_parsing(const char* ipstr, int port, int kinds){
    Aaddr addr; memset(&addr, 0, sizeof(Aaddr));

    struct addrinfo hints; memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    //IPv4 || IPv6
    hints.ai_socktype = 0;          //仅解析

    struct addrinfo* result = nullptr;
    if(getaddrinfo(ipstr, NULL, &hints, &result) != 0){
        aExcSet(AEXC_system_error);
        return addr;
    }

    //遍历所有解析结果
    for(struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next){
        memset(&addr, 0, sizeof(Aaddr));
        if(rp->ai_family == AF_INET){           // IPv4
            memcpy(&addr, rp->ai_addr, sizeof(struct sockaddr_in));
            addr.ip.sa_family = AF_INET;
            addr.ipv4.sin_port = port;
            addr.len = sizeof(struct sockaddr_in);
            addr.kinds = kinds;
        }else if(rp->ai_family == AF_INET6){    // IPv6
            memcpy(&addr, rp->ai_addr, sizeof(struct sockaddr_in6));
            addr.ip.sa_family = AF_INET6;
            addr.ipv6.sin6_port = port;
            addr.len = sizeof(struct sockaddr_in6);
            addr.kinds = kinds;
        }else{
            continue;
        }

        //测试连接
        Afd fd = A_INIT(Afd);
        switch(addr.kinds){
            case aip_tcp_client:
                fd = socket(addr.ip.sa_family, SOCK_STREAM, 0); break;
            case aip_udp_client:
                fd = socket(addr.ip.sa_family, SOCK_DGRAM, 0); break;
            case aip_raw_client:
                fd = socket(addr.ip.sa_family, SOCK_RAW, IPPROTO_RAW);
                if(Afd_exist(fd)){
                    int on = 1;
                    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
                }; break;
            default:
                fd = A_INIT(Afd); break;
        }
        if(Afd_exist(fd)){
            if(connect(fd, &addr.ip, addr.len) == 0){
                a_close(fd);
                break;
            }
        }else{
            memset(&addr, 0, sizeof(Aaddr));
            break;
        }
    }

    freeaddrinfo(result);

    return addr;
}

static inline Aaddr af_name_parsing(AStr name){
    Aaddr addr; memset(&addr, 0, sizeof(addr));
    char kinds[16]; memset(kinds, 0, 16); int port = 0, len = 0;
    if(sscanf(name.s, "%16[^|]|%d|%n", kinds, &port, &len) != 2){
        aExcSet(AEXC_outdomain);
        return addr;
    }
    const char* ipstr = name.s + len;
    if(!(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return addr;
    }
    port = htons(port);

    if(strcmp(kinds, "tcp_server") == 0){
        addr.kinds = aip_tcp_server;
    }else if(strcmp(kinds, "tcp_client") == 0){
        addr.kinds = aip_tcp_client;
    }else if(strcmp(kinds, "udp_server") == 0){
        addr.kinds = aip_udp_server;
    }else if(strcmp(kinds, "udp_client") == 0){
        addr.kinds = aip_udp_client;
    }else if(strcmp(kinds, "raw_server") == 0){
        addr.kinds = aip_raw_server;
    }else if(strcmp(kinds, "raw_client") == 0){
        addr.kinds = aip_raw_client;
    }else if(strcmp(kinds, "unix_server") == 0){
        addr.kinds = aip_unix_server;
    }else if(strcmp(kinds, "unix_client") == 0){
        addr.kinds = aip_unix_client;
    }else{
        aExcSet(AEXC_outdomain);
        return addr;
    }

    if(addr.kinds == aip_unix_server || addr.kinds == aip_unix_client){
        addr.ip.sa_family = AF_UNIX;
        addr.len = sizeof(struct sockaddr_un);
        strncpy(addr.unp.sun_path, ipstr, sizeof(addr.unp.sun_path) - 1);
        return addr;
    }

    if(inet_pton(AF_INET, ipstr, &addr.ipv4.sin_addr) == 1){
        addr.ip.sa_family = AF_INET;
        addr.ipv4.sin_port = port;
        addr.len = sizeof(struct sockaddr_in);
        return addr;
    }

    if(inet_pton(AF_INET6, ipstr, &addr.ipv6.sin6_addr) == 1){
        addr.ip.sa_family = AF_INET6;
        addr.ipv6.sin6_port = port;
        addr.len = sizeof(struct sockaddr_in6);
        return addr;
    }

    return af_domain_parsing(ipstr, port, addr.kinds);
}

static inline void af_socket_connect(Afd fd, Aaddr addr){
    if(!Afd_exist(fd)){
        aExcSet(AEXC_system_error);
        return;
    }

    int ret = 0;
    switch(addr.kinds){
        case aip_tcp_server: ret = bind(fd, &addr.ip, addr.len); if(ret >= 0) ret = listen(fd, SOMAXCONN); break;

        case aip_tcp_client: ret = connect(fd, &addr.ip, addr.len); break;

        case aip_udp_server: ret = bind(fd, &addr.ip, addr.len); break;

        case aip_udp_client: ret = connect(fd, &addr.ip, addr.len); break;

        case aip_unix_server: ret = bind(fd, &addr.ip, addr.len); if(ret >= 0) ret = listen(fd, SOMAXCONN); break;

        case aip_unix_client: ret = connect(fd, &addr.ip, addr.len); break;

        case aip_raw_server: ret = bind(fd, &addr.ip, addr.len); break;

        case aip_raw_client: ret = connect(fd, &addr.ip, addr.len); break;

        default: aExcSet(AEXC_outdomain); return; break;
    }

    if(ret < 0){
        aExcSet(AEXC_system_error);
    }
}

static inline Afd af_open_scoket(AStr name){
    Afd fd = A_INIT(Afd);
    aExcClean(); auto addr = af_name_parsing(name); if(aExcOccur()){
        return -1;
    }

    switch(addr.kinds){
        case aip_tcp_server:
            fd = socket(addr.ip.sa_family, SOCK_STREAM, 0);
            if(Afd_exist(fd)){
                int on = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
            }; break;
        case aip_tcp_client:
            fd = socket(addr.ip.sa_family, SOCK_STREAM, 0); break;
        case aip_udp_server: case aip_udp_client:
            fd = socket(addr.ip.sa_family, SOCK_DGRAM, 0); break;
        case aip_unix_server: case aip_unix_client:
            fd = socket(addr.ip.sa_family, SOCK_STREAM, 0); break;
        case aip_raw_server: case aip_raw_client:
            fd = socket(addr.ip.sa_family, SOCK_RAW, IPPROTO_RAW);
            if(Afd_exist(fd)){
                int on = 1;
                setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
            }; break;
        default:
            fd = A_INIT(Afd); break;
    }

    if(!Afd_exist(fd)){
        aExcSet(AEXC_system_error);
        return fd;
    }

    aExcClean(); af_socket_connect(fd, addr); if(aExcOccur()){
        if(Afd_exist(fd)) a_close(fd);
        fd = A_INIT(Afd);
    }

    return fd;
}



/*************************************************************************/
static inline int AFileNode_uplock_r(AFileNode* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }

    aExcClean(); AMtxRW_uplock_read(&self->lock); if(aExcOccur()){
        return -1;
    }

    aExcClean(); AMtx_uplock(&self->fl_lock); if(aExcOccur()){
        AMtxRW_unlock_read(&self->lock);
        return -1;
    }
    Apid pid = a_get_pid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false; self->rnum = 0;
    }
    if(!self->fl_stat && self->rnum == 0 && self->type != __aftype_socket){
        if(AFileLock_uplock_read(&self->filelock, self->fd) == -1){
            aExcSet(AEXC_system_error);
            AMtx_unlock(&self->fl_lock);
            AMtxRW_unlock_read(&self->lock);
            return -1;
        }
        self->fl_stat = true;
    }
    self->rnum++;
    AMtx_unlock(&self->fl_lock);

    return 0;
}

static inline int AFileNode_uplock_w(AFileNode* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }

    aExcClean(); AMtxRW_uplock_write(&self->lock); if(aExcOccur()){
        return -1;
    }

    aExcClean(); AMtx_uplock(&self->fl_lock); if(aExcOccur()){
        AMtxRW_unlock_write(&self->lock);
        return -1;
    }
    Apid pid = a_get_pid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false;
    }
    if(!self->fl_stat && self->type != __aftype_socket){
        if(AFileLock_uplock_write(&self->filelock, self->fd) == -1){
            aExcSet(AEXC_system_error);
            AMtx_unlock(&self->fl_lock);
            AMtxRW_unlock_write(&self->lock);
            return -1;
        }
        self->fl_stat = true;
    }
    AMtx_unlock(&self->fl_lock);

    return 0;
}

static inline int AFileNode_unlock_r(AFileNode* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }

    aExcClean(); AMtx_uplock(&self->fl_lock); if(aExcOccur()){
        return -1;
    }
    Apid pid = a_get_pid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false; self->rnum = 0;
    }
    if(self->rnum > 0) self->rnum--;
    if(self->fl_stat && self->rnum == 0 && self->type != __aftype_socket){
        AFileLock_unlock(&self->filelock, self->fd);
        self->fl_stat = false;
    }
    AMtx_unlock(&self->fl_lock);

    AMtxRW_unlock_read(&self->lock);
    return 0;
}

static inline int AFileNode_unlock_w(AFileNode* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }

    aExcClean(); AMtx_uplock(&self->fl_lock); if(aExcOccur()){
        return -1;
    }
    Apid pid = a_get_pid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false;
    }
    if(self->fl_stat && self->type != __aftype_socket){
        AFileLock_unlock(&self->filelock, self->fd);
        self->fl_stat = false;
    }
    AMtx_unlock(&self->fl_lock);

    AMtxRW_unlock_write(&self->lock);
    return 0;
}

static void AFileNode_open(AFileNode* self, int type, AStr name, int mod){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    self->name = A_COPY(AStr, name);
    self->mod = mod;
    self->type = type;

    int flag = 0;
    if((mod & __afmod_read) && !(mod & __afmod_write)) flag |= O_RDONLY;
    if((mod & __afmod_write) && !(mod & __afmod_read)) flag |= O_WRONLY;
    if((mod & __afmod_write) && (mod & __afmod_read)) flag |= O_RDWR;
    if(mod & __afmod_creat) { flag |= O_CREAT; if(!(mod & __afmod_appent)) flag |= O_TRUNC; }
    if(mod & __afmod_noblock) flag |= O_NONBLOCK;
    if(mod & __afmod_appent) flag |= O_APPEND;

    if(type == __aftype_file){
        self->fd = open(name.s, flag, 0644);
    }else if(type == __aftype_device){
        self->fd = open(name.s, flag);
    }else if(type == __aftype_socket){
        self->fd = af_open_scoket(name);
    }else{
        aExcSet(AEXC_outdomain);
        return;
    }

    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_system_error);
        A_DEST(AFileNode, *self);
        return;
    }

    if(mod & __afmod_exclusive){
        bool ret = true;
        aExcClean(); AMtxRW_uplock_write(&self->lock); if(aExcOccur()){
            ret = false;
        }

        if(ret && (AFileLock_uplock_write(&self->filelock, self->fd) == -1)){
            AMtxRW_unlock_write(&self->lock);
            ret = false;
        }

        if(ret) self->fl_stat = true;

        if(!ret){
            A_DEST(AFileNode, *self);
            aExcSet(AEXC_system_error);
        }
    }
}

static void AFileNode_set(AFileNode* self, Afd fd, int type, AStr name, int mod){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    self->name = A_COPY(AStr, name);
    self->mod = mod;
    self->type = type;
    self->fd = dup(fd);
    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_system_error);
    }
}

static uint32_t AFileNode_close(AFileNode* self, Afd fd){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 1;
    }
    if(!Afd_exist(fd)){
        return 1;
    }
    if((self->mod & __afmod_exclusive) || (self->fl_stat && self->num > 0)){
        auto ls = &self->fd_list;
        ls->f->pushBack(ls, fd);
    }else{
        a_close(fd);
    }

    if(self->num > 0){
        self->num--;
    }
    return self->num;
}

static int AFileNode_instantiation(AFileNode* self, int mod){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }

    if(!(self->mod & __afmod_write) && (self->mod & __afmod_read)){
        if(mod & __afmod_write){
            aExcSet(AEXC_system_error);
            return -1;
        }
    }
    if((self->mod & __afmod_write) && (self->mod & __afmod_appent)){
        if((mod & __afmod_write) && !(self->mod & __afmod_appent)){
            aExcSet(AEXC_system_error);
            return -1;
        }
    }
    if((self->mod & __afmod_read) && (self->mod & __afmod_noblock)){
        if(!((mod & __afmod_read) && (self->mod & __afmod_noblock))){
            aExcSet(AEXC_system_error);
            return -1;
        }
    }
    if(self->num > 0 && (self->mod & __afmod_exclusive)){
        aExcSet(AEXC_system_error);
        return -1;
    }

    int flag = 0;
    auto name = self->name;
    if((mod & __afmod_read) && !(mod & __afmod_write)) flag |= O_RDONLY;
    if((mod & __afmod_write) && !(mod & __afmod_read)) flag |= O_WRONLY;
    if((mod & __afmod_write) && (mod & __afmod_read)) flag |= O_RDWR;
    if(mod & __afmod_noblock) flag |= O_NONBLOCK;
    if(mod & __afmod_appent) flag |= O_APPEND;

    Afd fd = A_INIT(Afd);
    if((self->type == __aftype_file || self->type == __aftype_device) && self->num != 0){
        fd = open(name.s, flag);
    }else{
        fd = dup(self->fd);
    }

    if(!Afd_exist(fd)){
        aExcSet(AEXC_system_error);
        return -1;
    }

    self->num++;
    return fd;
}



/*************************************************************************/
static inline void afs_add(int type, AStr name, int mod){
    auto tab = &afstable;
    aExcClean(); RAII(AShPtr(AFileNode)) ptr = AShPtrNew(AFileNode);if(aExcOccur()){
        return;
    }
    AFileNode* node = ptr.p;

    AFileNode_open(node, type, name, mod);if(aExcOccur()){
        return;
    }

    tab->f->ins(tab, name, ptr);
}

static inline void afs_add_fd(Afd fd, int type, AStr name, int mod){
    auto tab = &afstable;

    aExcClean(); RAII(AShPtr(AFileNode)) ptr = AShPtrNew(AFileNode);if(aExcOccur()){
        return;
    }
    AFileNode* node = ptr.p;

    AFileNode_set(node, fd, type, name, mod);if(aExcOccur()){
        return;
    }

    tab->f->ins(tab, name, ptr);
}


static inline AShPtr(AFileNode)* afs_find(AStr name){
    auto tab = &afstable;
    auto ret = tab->f->at(tab, name);
    aExcClean();
    return ret;
}

static inline void afs_rm(AStr name){
    auto tab = &afstable;
    tab->f->rm(tab, name);
    aExcClean();
}

static inline bool afs_readable(AStr name){
    auto shp = afs_find(name);
    if(shp == nullptr){
        return true;
    }else{
        auto node = shp->p; auto mod = node->mod;
        if((mod & __afmod_read)){
            if((!(mod & __afmod_exclusive)) || node->tid == thrd_current()){
                return true;
            }
        }
    }
    return false;
}

static inline bool afs_writeable(AStr name){
    auto shp = afs_find(name);
    if(shp == nullptr){
        return true;
    }else{
        auto node = shp->p; auto mod = node->mod;
        if((mod & __afmod_write)){
            if((!(mod & __afmod_exclusive)) || node->tid == thrd_current()){
                return true;
            }
        }
    }
    return false;
}

static inline void afs_copy(AStr src, AStr tar){
    if(!afs_readable(src) || !afs_writeable(tar)){
        aExcSet(AEXC_system_error);
        return;
    }

    Afd src_fd = open(src.s, O_RDONLY);
    Afd tar_fd = open(tar.s, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if(!Afd_exist(src_fd) || !Afd_exist(tar_fd)){
        if(Afd_exist(src_fd)) a_close(src_fd);
        if(Afd_exist(tar_fd)) a_close(tar_fd);
        aExcSet(AEXC_system_error);
        return;
    }

    aExcClean();
    char buf[512]; memset(buf, 0, 512);
    auto info = __af_get_info(src.s);
    auto size = info.ast_size;
    uint64_t n = size / 512;
    if(aExcOccur()){
        if(Afd_exist(src_fd)) a_close(src_fd);
        if(Afd_exist(tar_fd)) a_close(tar_fd);
        return;
    }

    int ret = 0;
    for(uint64_t i = 0; i < n; i++){
        memset(buf, 0, 512);
        if(read(src_fd, buf, 512) != 512){
            ret = AEXC_system_error;
            break;
        }
        if(write(tar_fd, buf, 512) != 512){
            ret = AEXC_system_error;
            break;
        }
    }
    ssize_t x = size % 512;
    if(ret == 0){
        if(read(src_fd, buf, x) != x){
            ret = AEXC_system_error;
        }
    }
    if(ret == 0){
        if(write(tar_fd, buf, x) != x){
            ret = AEXC_system_error;
        }
    }

    if(Afd_exist(src_fd)) a_close(src_fd);
    if(Afd_exist(tar_fd)) a_close(tar_fd);
    if(ret != 0) aExcSet(ret);
}

static inline void afs_close(AStr name, Afd fd){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return;
    }

    auto ptrp = afs_find(name);
    if(__a_unlikely(ptrp == nullptr)){
        if(Afd_exist(fd)) a_close(fd);
        return;
    }
    auto node = ptrp->p;
    if(AFileNode_close(node, fd) == 0){
        afs_rm(name);
    }
}

static inline void afs_open(AFile* file, int type, AStr name, int32_t mod){
    if(__a_unlikely(file == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return;
    }

    auto ptrp = afs_find(name);
    if(__a_unlikely(ptrp == nullptr)){
        aExcClean();
        afs_add(type, name, mod);
        if(aExcOccur()){
            return;
        }
        ptrp = afs_find(name);
        if(__a_unlikely(ptrp == nullptr)){
            return;
        }
    }
    auto node = ptrp->p;
    file->fd = AFileNode_instantiation(node, mod);
    if(!Afd_exist(file->fd)){
        return;
    }

    aExcClean();
    file->name = A_COPY(AStr, name);
    file->node = A_COPY(AShPtr(AFileNode), *ptrp);
    if(aExcOccur()){
        A_DEST(AFile, *file);
        file->fd = A_INIT(Afd);
    }

    return;
}

static inline void afs_register(AFile* file, int fd, int type, AStr name, int32_t mod){
    if(__a_unlikely(file == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(!Afd_exist(fd)){
        aExcSet(AEXC_init_failed);
        return;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return;
    }

    auto ptrp = afs_find(name);
    if(__a_unlikely(ptrp != nullptr)){
        aExcSet(AEXC_repeat_write);
        return;
    }

    aExcClean();
    afs_add_fd(fd, type, name, mod);
    if(aExcOccur()){
        return;
    }
    ptrp = afs_find(name);
    if(__a_unlikely(ptrp == nullptr)){
        return;
    }

    auto ptr = *ptrp;
    file->fd = fd;

    aExcClean();
    file->name = A_COPY(AStr, name);
    file->node = A_COPY(AShPtr(AFileNode), ptr);
    if(aExcOccur()){
        A_DEST(AFile, *file);
        file->fd = A_INIT(Afd);
    }
    return;
}

static inline int afs_ioctl(AShPtr(AFileNode) ptr, Afd fd, int cmd, void* buf){
    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || !Afd_exist(fd))){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return 0;
    }
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return 0;
    }

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_uplock_w(node) != 0){
            aExcSet(AEXC_system_error);
            return 0;
        }
    }
    A_DEST(AAutoKey, key);

    int ret = ioctl(fd, cmd, buf);

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_unlock_w(node) != 0){
            aExcSet(AEXC_system_error);
            return ret;
        }
    }

    return ret;
}

static inline int afs_read(AShPtr(AFileNode) ptr, Afd fd, void* target, size_t size){
    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || !Afd_exist(fd))){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return 0;
    }

    if(!afsflag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    if(!(node->mod & __afmod_read)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_uplock_r(node) != 0){
            aExcSet(AEXC_system_error);
            return 0;
        }
    }
    A_DEST(AAutoKey, key);

    int ret = read(fd, target, size);

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_unlock_r(node) != 0){
            aExcSet(AEXC_system_error);
            return ret;
        }
    }

    return ret;
}

static inline int afs_read_pos(AShPtr(AFileNode) ptr, Afd fd, uint64_t offset, void* target, size_t size){
    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || !Afd_exist(fd))){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return 0;
    }
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    if(!(node->mod & __afmod_read)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_uplock_r(node) != 0){
            aExcSet(AEXC_system_error);
            return 0;
        }
    }
    A_DEST(AAutoKey, key);

    int ret = af_pread(fd, target, size, offset);

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_unlock_r(node) != 0){
            aExcSet(AEXC_system_error);
            return ret;
        }
    }

    return ret;
}

static inline int afs_write(AShPtr(AFileNode) ptr, Afd fd, void* source, size_t size){
    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || !Afd_exist(fd))){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return 0;
    }
    if(!afsflag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    if(!(node->mod & __afmod_write)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_uplock_w(node) != 0){
            aExcSet(AEXC_system_error);
            return 0;
        }
    }
    A_DEST(AAutoKey, key);

    int ret = write(fd, source, size);

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_unlock_w(node) != 0){
            aExcSet(AEXC_system_error);
            return ret;
        }
    }

    return ret;
}

static inline int afs_write_pos(AShPtr(AFileNode) ptr, Afd fd, uint64_t offset, void* source, size_t size){
    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || !Afd_exist(fd))){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return 0;
    }
    if(!(node->mod & __afmod_write)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    if(!afsflag){
        aExcSet(AEXC_system_error);
        return 0;
    }

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_uplock_w(node) != 0){
            aExcSet(AEXC_system_error);
            return 0;
        }
    }
    A_DEST(AAutoKey, key);

    int ret = af_pwrite(fd, source, size, offset);

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_unlock_w(node) != 0){
            aExcSet(AEXC_system_error);
            return ret;
        }
    }

    return ret;
}


/*************************************************************************/
int32_t  AFile_ioctl(AFile* self, int32_t cmd, void* buf){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = afs_ioctl(self->node, self->fd, cmd, buf);
    if(ret < 0){
        aExcSet(AEXC_system_error);
        ret = 0;
    }
    return (uint32_t)ret;
}

uint32_t AFile_read (AFile* self, size_t size, void* target){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = afs_read(self->node, self->fd, target, size);
    if(ret < 0){
        aExcSet(AEXC_system_error);
        ret = 0;
    }
    return (uint32_t)ret;
}

uint32_t AFile_write(AFile* self, size_t size, void* source){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = afs_write(self->node, self->fd, source, size);
    if(ret < 0){
        aExcSet(AEXC_system_error);
        ret = 0;
    }
    return (uint32_t)ret;
}

uint32_t AFile_read_pos(AFile* self, uint64_t offset, size_t size, void* target){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = afs_read_pos(self->node, self->fd, offset, target, size);
    if(ret < 0){
        aExcSet(AEXC_system_error);
        ret = 0;
    }
    return (uint32_t)ret;
}

uint32_t AFile_write_pos(AFile* self, uint64_t offset, size_t size, void* source){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = afs_write_pos(self->node, self->fd, offset, source, size);
    if(ret < 0){
        aExcSet(AEXC_system_error);
        ret = 0;
    }
    return (uint32_t)ret;
}

void AFile_close(AFile* self){
    if(self == nullptr || !Afd_exist(self->fd)){
        return;
    }
    aExcClean();
    afs_close(self->name, self->fd);
    if(aExcOccur()){
        if(Afd_exist(self->fd)) a_close(self->fd);
    }

    A_DEST(AShPtr(AFileNode), self->node);
    A_DEST(AStr, self->name);
    memset(self, 0, sizeof(AFile));
    self->fd = A_INIT(Afd);
}

void AFile_register(AFile* self, Afd fd, int type, AStr name, int mod){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(Afd_exist(self->fd)){
        aExcSet(AEXC_init_failed);
        return;
    }

    if(!(mod & __afmod_read)){ mod &= ~__afmod_noblock; }
    if(!(mod & __afmod_write)){ mod &= ~__afmod_creat; }
    if(!(mod & __afmod_write)){ mod &= ~__afmod_exclusive; }
    switch(type){
        case __aftype_file:  break;
        case __aftype_device:break;
        case __aftype_socket:break;
        default: aExcSet(AEXC_outdomain); return; break;
    }

    afs_register(self, fd, type, name, mod);
}

void AFile_open(AFile* self, int type, AStr name, int mod){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(Afd_exist(self->fd) || mod == 0 || type == 0){
        aExcSet(AEXC_init_failed);
        return;
    }

    if(!(mod & __afmod_read)){ mod &= ~__afmod_noblock; }
    if(!(mod & __afmod_write)){ mod &= ~__afmod_creat; }
    if(!(mod & __afmod_write)){ mod &= ~__afmod_exclusive; }
    switch(type){
        case __aftype_file:  break;
        case __aftype_device:break;
        case __aftype_socket:break;
        default: aExcSet(AEXC_outdomain); return; break;
    }

    afs_open(self, type, name, mod);
}



/*************************************************************************/
AFile aFileInOpen(const char* name){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AStr) path = __af_path_absolute(name);

    int mod = __afmod_read;
    AFile_open(&file, __aftype_file, path, mod);
    return file;
}
AFile aFileOutOpen(const char* name){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AStr) path = __af_path_absolute(name);

    int mod = __afmod_write | __afmod_exclusive | __afmod_creat | __afmod_truncate;
    AFile_open(&file, __aftype_file, path, mod);
    return file;
}
AFile aFileEndOpen(const char* name){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AStr) path = __af_path_absolute(name);

    int mod = __afmod_write | __afmod_appent | __afmod_creat;
    AFile_open(&file, __aftype_file, path, mod);
    return file;
}

AFile aDevInOpen(const char* name, bool noblock){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AStr) path = __af_path_absolute(name);

    int mod = __afmod_read;
    if(noblock) mod |= __afmod_noblock;
    AFile_open(&file, __aftype_device, path, mod);
    return file;
}
AFile aDevOutOpen(const char* name, bool exclusive){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AStr) path = __af_path_absolute(name);

    int mod = __afmod_write;
    if(exclusive) mod |= __afmod_exclusive;
    AFile_open(&file, __aftype_device, path, mod);
    return file;
}
AFile aDevInOutOpen(const char* name, bool noblock, bool exclusive){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AStr) path = __af_path_absolute(name);

    int mod = __afmod_read | __afmod_write;
    if(noblock) mod |= __afmod_noblock;
    if(exclusive) mod |= __afmod_exclusive;
    AFile_open(&file, __aftype_device, path, mod);
    return file;
}

AFile aSocketTcpServerOpen(const char* ipstr, int port){
    AFile file = A_INIT(AFile);

    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "tcp_server";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}
AFile aSocketUdpServerOpen(const char* ipstr, int port){
    AFile file = A_INIT(AFile);

    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "udp_server";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}
AFile aSocketUnixServerOpen(const char* ipstr){
    AFile file = A_INIT(AFile);

    int port = 0;
    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "unix_server";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}
AFile aSocketRawServerOpen(const char* ipstr, int port){
    AFile file = A_INIT(AFile);

    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "raw_server";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}
AFile aSocketTcpClientOpen(const char* ipstr, int port){
    AFile file = A_INIT(AFile);

    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "tcp_client";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}
AFile aSocketUdpClientOpen(const char* ipstr, int port){
    AFile file = A_INIT(AFile);

    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "udp_client";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}
AFile aSocketUnixClientOpen(const char* ipstr){
    AFile file = A_INIT(AFile);

    int port = 0;
    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "unix_client";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}
AFile aSocketRawClientOpen(const char* ipstr, int port){
    AFile file = A_INIT(AFile);

    char buf[8]; memset(buf, 0, sizeof(buf));
    if((ipstr == nullptr) || !(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", port);
    const char* kinds = "raw_client";

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, name, mod);
    return file;
}

static inline const char* af_getip(AFile file){
    if(!Afd_exist(file.fd)){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }
    auto node = file.node.p;
    if(node != nullptr && node->type != __aftype_socket){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }
    auto name = node->name;

    char kinds[16]; memset(kinds, 0, 16); int port = 0, len = 0;
    if(sscanf(name.s, "%16[^|]|%d|%n", kinds, &port, &len) != 2){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }
    const char* ipstr = name.s + len;
    if(!(0 <= port && port < (int)(1 << 16))){
        aExcSet(AEXC_outdomain);
        return nullptr;
    }
    port = htons(port);

    if(strcmp(kinds, "tcp_server") == 0){
    }else if(strcmp(kinds, "tcp_client") == 0){
    }else if(strcmp(kinds, "udp_server") == 0){
    }else if(strcmp(kinds, "udp_client") == 0){
    }else if(strcmp(kinds, "raw_server") == 0){
    }else if(strcmp(kinds, "raw_client") == 0){
    }else if(strcmp(kinds, "unix_server") == 0){
    }else if(strcmp(kinds, "unix_client") == 0){
    }else{
        aExcSet(AEXC_outdomain);
        return nullptr;
    }

    return ipstr;
}


static inline AFile aSocketAccept(AFile server, const char* kinds){
    AFile file = A_INIT(AFile);
    static atomic_int accept_num = 0;

    char buf[8]; memset(buf, 0, sizeof(buf));
    const char* ipstr = af_getip(server);
    if(ipstr == nullptr){
        return file;
    }
    snprintf(buf, sizeof(buf), "|%d|", atomic_fetch_add(&accept_num, 1));

    aExcClean();
    RAII(AStr) name = A_INIT(AStr);
    AStr_addBack(&name, kinds);
    AStr_addBack(&name, buf);
    AStr_addBack(&name, ipstr);
    if(aExcOccur()){
        return file;
    }

    socklen_t len = sizeof(Aaddr);
    Aaddr addr; memset(&addr, 0, len);
    Afd fd = accept(server.fd, &addr.ip, &len);
    if(!Afd_exist(fd)){
        aExcSet(AEXC_system_error);
        return file;
    }

    aExcClean();
    int mod = __afmod_read | __afmod_write;
    AFile_register(&file, fd, __aftype_socket, name, mod);
    if(aExcOccur()){
        a_close(fd);
    }

    return file;
}
AFile aSocketTcpAccept(AFile server){
    return aSocketAccept(server, "tcp_accept");
}
AFile aSocketUnixAccept(AFile server){
    return aSocketAccept(server, "unix_accept");
}



/*************************************************************************/
static AFileInfo __af_get_info(const char* name){
    AFileInfo info = {0};
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return info;
    }

    struct stat st;
    if(stat(name, &st) != 0){
        aExcSet(AEXC_system_error);
        return info;
    }

    info.ast_dev = (uint64_t)st.st_dev;
    info.ast_ino = (uint64_t)st.st_ino;
    info.ast_mode = (uint32_t)st.st_mode;
    info.ast_nlink = (uint32_t)st.st_nlink;
    info.ast_uid = (uint32_t)st.st_uid;
    info.ast_gid = (uint32_t)st.st_gid;
    info.ast_rdev = (uint64_t)st.st_rdev;
    info.ast_size = (uint64_t)st.st_size;
    info.ast_atime = (stat_time_t)st.st_atim.tv_sec;
    info.ast_mtime = (stat_time_t)st.st_mtim.tv_sec;
    info.ast_ctime = (stat_time_t)st.st_ctim.tv_sec;
    return info;
}

static bool __af_path_exist(const char* name){
    struct stat st;
    return name != nullptr && stat(name, &st) == 0;
}

static bool __af_isfile(const char* name){
    struct stat st;
    return name != nullptr && stat(name, &st) == 0 && S_ISREG(st.st_mode);
}
static bool __af_isdir(const char* name){
    struct stat st;
    return name != nullptr && stat(name, &st) == 0 && S_ISDIR(st.st_mode);
}
static bool __af_isdev(const char* name){
    struct stat st;
    return name != nullptr && stat(name, &st) == 0 &&
        (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode));
}

static void __af_mkdir(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__af_isdir(name)){
        return;
    }

    aExcClean();
    RAII(AStr) all = A_INIT(AStr);
    if(aExcOccur()){
        return;
    }

    if(name[0] == '/'){
        aExcClean();
        AStr_addBack(&all, name);
        if(aExcOccur()){
            return;
        }
    }else{
        aExcClean();
        RAII(AStr) text = __af_path_absolute(".");
        AStr_addBack(&all, text.s);
        AStr_pushBack(&all, '/');
        AStr_addBack(&all, name);
        if(aExcOccur()){
            return;
        }
    }

    aExcClean(); RAII(AStr) path = __af_dir_extract(all.s); if(aExcOccur()){
        return;
    }

    if(!__af_path_exist(path.s)){
        aExcClean(); __af_mkdir(path.s); if(aExcOccur()){
            return;
        }
    }

    if(__af_isdir(path.s)){
        if(mkdir(all.s, 0755) != 0){
            aExcSet(AEXC_system_error);
            return;
        }
    }else{
        aExcSet(AEXC_repeat_write);
        return;
    }
}

static void __af_rm(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(!__af_isfile(name)){
        aExcSet(AEXC_outdomain);
        return;
    }

    if(remove(name) != 0){
        aExcSet(AEXC_system_error);
    }
}
static void __af_rm_r(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(strcmp(name, "/") == 0){
        aExcSet(AEXC_outdomain);
        return;
    }
    if(__af_isfile(name)){
        __af_rm(name);
    }else if(__af_isdir(name)){
        RAII(ALine(AStr)) list = __af_ls(name);
        forEach(it, list){
            AStr* x = it.p;
            if(x != nullptr){
                aExcClean(); __af_rm_r(x->s); if(aExcOccur()){
                    return;
                }
            }
        }
        if(rmdir(name) != 0){
            aExcSet(AEXC_system_error);
            return;
        }
    }else{
        aExcSet(AEXC_outdomain);
    }
}

static void __af_cp(const char* name, const char* target){
    if(name == nullptr || target == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(!__af_isfile(name)){
        aExcSet(AEXC_outdomain);
        return;
    }

    RAII(AStr) src = __af_path_absolute(name);
    RAII(AStr) tar = A_INIT(AStr);

    if(__af_isdir(target)){
        aExcClean();
        RAII(AStr) fn = __af_name_extract(name);
        RAII(AStr) pt = __af_path_absolute(target);

        AStr_addBack(&tar, pt.s);
        if(AStr_at(&tar, AEND) != '/'){
            AStr_pushBack(&tar, '/');
        }
        AStr_addBack(&tar, fn.s);
        if(AStr_at(&tar, AEND) == '/'){
            AStr_popBack(&tar);
        }

        if(aExcOccur()){
            return;
        }
    }else if(target[0] == '/'){
        //绝对路径
        aExcClean();
        AStr_addBack(&tar, target);
        if(aExcOccur()){
            return;
        }
    }else{
        //文件名
        aExcClean();
        RAII(AStr) pt = __af_dir_extract(src.s);

        AStr_addBack(&tar, pt.s);
        if(AStr_at(&tar, AEND) != '/'){
            AStr_pushBack(&tar, '/');
        }
        AStr_addBack(&tar, target);
        if(AStr_at(&tar, AEND) == '/'){
            AStr_popBack(&tar);
        }

        if(aExcOccur()){
            return;
        }
    }

    afs_copy(src, tar);
}
static void __af_cp_r(const char* name, const char* target){
    if(name == nullptr || target == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(strcmp(name, "/") == 0){
        aExcSet(AEXC_outdomain);
        return;
    }
    if(__af_isfile(name)){
        __af_cp(name, target);
        return;
    }

    if(__af_path_exist(target) && !__af_isdir(target)){
        aExcSet(AEXC_outdomain);
        return;
    }

    aExcClean();
    RAII(AStr) tar = AStr_new(target);
    {
        if(AStr_at(&tar, AEND) != '/'){
            AStr_pushBack(&tar, '/');
        }
    }
    if(__af_path_exist(target)){
        RAII(AStr) src = __af_name_extract(name);
        AStr_addBack(&tar, src.s);
        if(AStr_at(&tar, AEND) != '/'){
            AStr_pushBack(&tar, '/');
        }
    }
    __af_mkdir(tar.s);
    RAII(ALine(AStr)) list = __af_ls(name);
    if(aExcOccur()){
        return;
    }

    forEach(it, list){
        AStr* entry = it.p;
        if(entry != nullptr){
            aExcClean(); __af_cp_r(entry->s, tar.s); if(aExcOccur()){
                return;
            }
        }
    }
}

static void __af_mv(const char* name, const char* new_name){
    if(name == nullptr || new_name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    RAII(AStr) tar = AStr_new(new_name);
    if(__af_isdir(new_name)){
        aExcClean();
        RAII(AStr) src = __af_name_extract(name);
        if(AStr_at(&tar, AEND) != '/'){
            AStr_pushBack(&tar, '/');
        }
        AStr_addBack(&tar, src.s);
        if(aExcOccur()){
            return;
        }
    }
    if(rename(name, tar.s) != 0){
        aExcSet(AEXC_system_error);
    }
}

static void __af_touch(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__af_path_exist(name)){
        if(!__af_isfile(name) || __af_isdev(name)){
            aExcSet(AEXC_outdomain);
        }
        return;
    }
    Afd fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(!Afd_exist(fd)){
        aExcSet(AEXC_system_error);
        return;
    }
    a_close(fd);
}

static void __af_chmod(const char* name, int p){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(!__af_isfile(name)){
        aExcSet(AEXC_outdomain);
        return;
    }
    if(chmod(name, p) != 0){
        aExcSet(AEXC_system_error);
    }
}
static void __af_chmod_r(const char* name, int p){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__af_isfile(name)){
        __af_chmod(name, p);
        return;
    }else if(__af_isdir(name)){
        if(chmod(name, p) != 0){
            aExcSet(AEXC_system_error);
            return;
        }
        RAII(ALine(AStr)) list = __af_ls(name);
        forEach(it, list){
            AStr* x = it.p;
            if(x != nullptr){
                aExcClean(); __af_chmod_r(x->s, p); if(aExcOccur()){
                    return;
                }
            }
        }
    }else{
        aExcSet(AEXC_outdomain);
    }
}

static ALine(AStr) __af_ls(const char* name){
    ALine(AStr) list = A_INIT(ALine(AStr));

    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return list;
    }
    if(__af_isfile(name)){
        list.f->pushBack(&list, AStr_new(name));
        return list;
    }
    if(!__af_isdir(name)){
        aExcSet(AEXC_outdomain);
        return list;
    }

    aExcClean(); RAII(AStr) path = __af_path_absolute(name); if(aExcOccur()){
        return list;
    }

    DIR* dir = opendir(name);
    if(dir == nullptr){
        aExcSet(AEXC_system_error);
        return list;
    }

    errno = 0;
    struct dirent *entry;
    while((entry = readdir(dir)) != nullptr){
        // 跳过 "." 和 ".."
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        aExcClean();
        RAII(AStr) text = A_COPY(AStr, path);
        AStr_pushBack(&text, '/');
        AStr_addBack(&text, entry->d_name);
        if(aExcOccur()){
            break;
        }

        aExcClean();
        if(AStr_at(&text, AEND) == '/'){
            AStr_popBack(&text);
        }
        if(aExcOccur()){
            break;
        }

        list.f->pushBack(&list, text);
        if(aExcOccur()){
            break;
        }
    }

    if(errno != 0){
        aExcSet(AEXC_system_error);
        errno = 0;
    }
    closedir(dir);

    return list;
}

static AStr __af_path_absolute(const char* name){
    char buf[PATH_MAX]; memset(buf, 0, PATH_MAX);
    AStr text = A_INIT(AStr);
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return text;
    }

    aExcClean();
    if(name[0] == '/'){
        AStr_addBack(&text, name);
    }else{
        if(realpath(".", buf) == nullptr){
            aExcSet(AEXC_system_error);
            return text;
        }

        AStr_addBack(&text, buf);
        if(AStr_at(&text, AEND) != '/'){
            AStr_pushBack(&text, '/');
        }
        AStr_addBack(&text, name);
    }

    if(__af_isdir(text.s)){
        if(AStr_at(&text, AEND) == '/'){
            AStr_popBack(&text);
        }
    }

    if(aExcOccur()){
        return A_INIT(AStr);
    }

    return text;
}

static AStr __af_name_extract(const char* name){
    AStr text = A_INIT(AStr);
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return text;
    }
    if(strlen(name) == 0){
        aExcSet(AEXC_outdomain);
        return text;
    }
    if(strcmp(name, "/") == 0){
        AStr_addBack(&text, name);
        return text;
    }

    const char* p = name + strlen(name) - 1;

    if(*p == '/') p--;
    while(p != name){
        if(*p == '/' && *(p-1) != '\\'){
            break;
        }
        p--;
    }

    if(*p == '/') p++;
    aExcClean(); AStr_addBack(&text, p); if(aExcOccur()){
        return text;
    }

    if(AStr_at(&text, AEND) == '/'){
        AStr_popBack(&text);
    }

    return text;
}

static AStr __af_dir_extract(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return A_INIT(AStr);
    }
    if(strcmp(name, "/") == 0){
        AStr path = A_INIT(AStr);
        AStr_addBack(&path, name);
        return path;
    }

    AStr path = A_INIT(AStr);
    if(name[0] == '/'){
        aExcClean();
        AStr_addBack(&path, name);
        if(aExcOccur()){
            return A_INIT(AStr);
        }
    }else{
        aExcClean();
        RAII(AStr) all = __af_path_absolute(name);
        AStr_addBack(&path, all.s);
        if(aExcOccur()){
            return A_INIT(AStr);
        }
    }

    char* p = path.s + AStr_getNumber(&path); p--;
    if(*p == '/') p++;
    while(p != path.s){
        if(*p == '/' && *(p-1) != '\\'){
            break;
        }
        p--;
    }
    AStr_truncate(&path, (uint32_t)(p - path.s));

    if(AStr_at(&path, AEND) == '/'){
        AStr_popBack(&path);
    }
    return path;
}



/*************************************************************************/
void af_mkdir(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_mkdir(name);
}

void af_rm(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_rm(name);
}
void af_rm_r(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_rm_r(name);
}

void af_cp(const char* name, const char* target){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_cp(name, target);
}
void af_cp_r(const char* name, const char* target){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_cp_r(name, target);
}

void af_mv(const char* name, const char* new_name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_mv(name, new_name);
}

void af_touch(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_touch(name);
}

void af_chmod(const char* name, int p){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_chmod(name, p);
}
void af_chmod_r(const char* name, int p){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&afslock); if(aExcOccur()){
        return;
    }
    __af_chmod_r(name, p);
}

bool af_path_exist(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return false;
    }
    return __af_path_exist(name);
}
bool af_isfile(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return false;
    }
    return __af_isfile(name);
}
bool af_isdir(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return false;
    }
    return __af_isdir(name);
}
bool af_isdev(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return false;
    }
    return __af_isdev(name);
}

AStr af_dir_extract(const char* name){
    return __af_dir_extract(name);
}

AStr af_name_extract(const char* name){
    return __af_name_extract(name);
}

AStr af_path_absolute(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return A_INIT(AStr);
    }
    return __af_path_absolute(name);
}

ALine(AStr) af_ls(const char* dir){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return A_INIT(ALine(AStr));
    }
    return __af_ls(dir);
}
AFileInfo af_get_info(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return A_INIT(AFileInfo);
    }
    return __af_get_info(name);
}



#endif /* posix */
