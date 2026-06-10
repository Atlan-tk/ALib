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

#ifdef st_atime
#undef st_atime
#endif
#ifdef st_mtime
#undef st_mtime
#endif
#ifdef st_ctime
#undef st_ctime
#endif



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
static AString __af_dir_extract(const char* name);
static AString __af_name_extract(const char* name);
static AString __af_path_absolute(const char* name);
static AFileInfo __af_get_info(const char* name);
static ALine(AString) __af_ls(const char* dir);



/*************************************************************************/
typedef struct{
    AString     addr;       //"192.168.0.1","/tmp/test"
    int         family;     //AF_INET, AF_INET6, AF_UNIX
    int         kinds;      //tcp_server, tcp_client, udp_server, ...
    uint16_t    port;       //port
}AIpPort;
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

static inline AIpPort af_name_parsing(__noused AString name){
    AIpPort ipport = {}; memset(&ipport, 0, sizeof(AIpPort));

    RAII(AString) addr = A_INIT(AString);
    int family = 0, kinds = 0; uint64_t port = 0;

    auto p = name.s;
    if(strncmp(p, "tcp|server|", 11) == 0){
        kinds = aip_tcp_server, p += 11;
    }else if(strncmp(p, "tcp|client|", 11) == 0){
        kinds = aip_tcp_client, p += 11;
    }else if(strncmp(p, "udp|server|", 11) == 0){
        kinds = aip_udp_server, p += 11;
    }else if(strncmp(p, "udp|client|", 11) == 0){
        kinds = aip_udp_client, p += 11;
    }else if(strncmp(p, "raw|server|", 11) == 0){
        kinds = aip_raw_server, p += 11;
    }else if(strncmp(p, "raw|client|", 11) == 0){
        kinds = aip_raw_client, p += 11;
    }else if(strncmp(p, "unix|server|", 12) == 0){
        kinds = aip_unix_server, p += 12;
    }else if(strncmp(p, "unix|client|", 12) == 0){
        kinds = aip_unix_client, p += 12;
    }else{
        aExcSet(AEXC_outdomain);
        return ipport;
    }

    if(kinds == aip_unix_server || kinds == aip_unix_client){
        family = AF_UNIX;
        addr.f->addBack(&addr, AString_new(p));
    }else{
        char buf[64]; memset(buf, 0, 64);
        int addr_len = 0;
        if(sscanf(p, "%63[^|]|%lu%n", buf, &port, &addr_len) != 2){
            aExcSet(AEXC_outdomain);
            return ipport;
        }
        p += addr_len;

        if(p[0] == 0){
        }else if(p[0] == '|' && '0' <= p[1] && p[1] <= '9'){
            p++; uint64_t end = 0; int end_len = 0;
            if(sscanf(p, "%lu%n", &end, &end_len) == 1 && p[end_len] == '\0'){
            }else{
                aExcSet(AEXC_outdomain);
                return ipport;
            }
        }else{
            aExcSet(AEXC_outdomain);
            return ipport;
        }

        if(port >= (uint64_t)(1 << 16)){
            aExcSet(AEXC_outdomain);
            return ipport;
        }

        {
            struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
            if(inet_pton(AF_INET, buf, &addr.sin_addr) == 1){
                family = AF_INET;
            }else{
                struct sockaddr_in6 addr6; memset(&addr6, 0, sizeof(addr6));
                if(inet_pton(AF_INET6, buf, &addr6.sin6_addr) == 1){
                    family = AF_INET6;
                }else{
                    aExcSet(AEXC_outdomain);
                    return ipport;
                }
            }
        }

        addr.f->addBack(&addr, AString_new(buf));
    }

    ipport.family = family;
    ipport.addr = A_MOVE(addr);
    ipport.kinds = kinds;
    ipport.port = port;
    ipport.port = htons(ipport.port);

    return ipport;
}

static inline void af_socket_connect(Afd fd, AIpPort ipport){
    if(!Afd_exist(fd)){
        aExcSet(AEXC_system_error);
        return;
    }

    int ret = 0;

    int size = 0; struct sockaddr* addr = nullptr;
    struct sockaddr_in ip_addr = {
        .sin_family = ipport.family,
        .sin_port = ipport.port,
    };
    struct sockaddr_in6 ip_addr6 = {
        .sin6_family = ipport.family,
        .sin6_port = ipport.port,
    };
    struct sockaddr_un un_addr = {
        .sun_family = ipport.family,
    };

    if(ipport.family == AF_INET6){
        if(inet_pton(AF_INET6, ipport.addr.s, &ip_addr6.sin6_addr) < 0){
            aExcSet(AEXC_system_error);
            return;
        }
        addr = (void*)&ip_addr6, size = sizeof(ip_addr6);
    }else if(ipport.family == AF_INET){
        if(inet_pton(AF_INET, ipport.addr.s, &ip_addr.sin_addr) < 0){
            aExcSet(AEXC_system_error);
            return;
        }
        addr = (void*)&ip_addr, size = sizeof(ip_addr);
    }else if(ipport.family == AF_UNIX){
        strncpy(un_addr.sun_path, ipport.addr.s, sizeof(un_addr.sun_path) - 1);
        addr = (void*)&un_addr, size = sizeof(un_addr);
    }

    switch(ipport.kinds){
        case aip_tcp_server: ret = bind(fd, addr, size); if(ret >= 0) ret = listen(fd, SOMAXCONN); break;

        case aip_tcp_client: ret = connect(fd, addr, size); break;

        case aip_udp_server: ret = bind(fd, addr, size); break;

        case aip_udp_client: ret = connect(fd, addr, size); break;

        case aip_unix_server: ret = bind(fd, addr, size); if(ret >= 0) ret = listen(fd, SOMAXCONN); break;

        case aip_unix_client: ret = connect(fd, addr, size); break;

        case aip_raw_server: ret = bind(fd, addr, size); break;

        case aip_raw_client: ret = connect(fd, addr, size); break;

        default: aExcSet(AEXC_outdomain); return; break;
    }

    if(ret < 0){
        aExcSet(AEXC_system_error);
    }
}

static inline Afd af_open_scoket(AString name){
    Afd fd = A_INIT(Afd);
    aExcClean(); auto ipport = af_name_parsing(name); if(aExcOccur()){
        return -1;
    }

    switch(ipport.kinds){
        case aip_tcp_server:
            fd = socket(ipport.family, SOCK_STREAM, 0);
            if(Afd_exist(fd)){
                int on = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
            }; break;
        case aip_tcp_client:
            fd = socket(ipport.family, SOCK_STREAM, 0); break;
        case aip_udp_server: case aip_udp_client:
            fd = socket(ipport.family, SOCK_DGRAM, 0); break;
        case aip_unix_server: case aip_unix_client:
            fd = socket(ipport.family, SOCK_STREAM, 0); break;
        case aip_raw_server: case aip_raw_client:
            fd = socket(ipport.family, SOCK_RAW, IPPROTO_RAW);
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

    aExcClean(); af_socket_connect(fd, ipport); if(aExcOccur()){
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

static void AFileNode_open(AFileNode* self, int type, AString name, int mod){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    self->name = A_COPY(AString, name);
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

static void AFileNode_set(AFileNode* self, Afd fd, int type, AString name, int mod){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    self->name = A_COPY(AString, name);
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
AHash_Define(AString,AShPtr(AFileNode));
AHash_Generate(AString,AShPtr(AFileNode));
A_TYPE_REGISTER(AHash(AString,AShPtr(AFileNode)));

static bool         afsflag = false;
static AMtxRW       afslock;
static AHash(AString,AShPtr(AFileNode)) afstable;

__attribute__((constructor)) static inline void afs_start(){
    aExcClean(); afstable = A_INIT(AHash(AString,AShPtr(AFileNode))); if(aExcOccur()){
        return;
    }
    aExcClean(); afslock = A_INIT(AMtxRW);if(aExcOccur()){
        return;
    }
    afsflag = true;
}
__attribute__((destructor)) static inline void afs_poweroff(){
    afsflag = false;
    A_DEST(AHash(AString,AShPtr(AFileNode)), afstable);
    A_DEST(AMtxRW, afslock);
}

static inline void afs_add(int type, AString _name, int mod){
    aExcClean();
    RAII(AString) name = A_INIT(AString); name.f->addBack(&name, _name);
    if(aExcOccur()){
        return;
    }
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

static inline void afs_add_fd(Afd fd, int type, AString _name, int mod){
    aExcClean();
    RAII(AString) name = A_INIT(AString); name.f->addBack(&name, _name);
    if(aExcOccur()){
        return;
    }

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


static inline AShPtr(AFileNode)* afs_find(AString name){
    auto tab = &afstable;
    auto ret = tab->f->at(tab, name);
    aExcClean();
    return ret;
}

static inline void afs_rm(AString name){
    auto tab = &afstable;
    tab->f->rm(tab, name);
    aExcClean();
}

static inline bool afs_readable(AString name){
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

static inline bool afs_writeable(AString name){
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

static inline void afs_copy(AString src, AString tar){
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
    auto size = info.st_size;
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

static inline void afs_close(AString name, Afd fd){
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

static inline void afs_open(AFile* file, int type, const char* name, int32_t mod){
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

    auto ptrp = afs_find(AString_new(name));
    if(__a_unlikely(ptrp == nullptr)){
        aExcClean();
        afs_add(type, AString_new(name), mod);
        if(aExcOccur()){
            return;
        }
        ptrp = afs_find(AString_new(name));
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
    file->name = A_INIT(AString); file->name.f->addBack(&file->name, AString_new(name));
    file->node = A_COPY(AShPtr(AFileNode), *ptrp);
    if(aExcOccur()){
        A_DEST(AFile, *file);
        file->fd = A_INIT(Afd);
    }

    return;
}

static inline void afs_register(AFile* file, int fd, int type, const char* name, int32_t mod){
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

    auto ptrp = afs_find(AString_new(name));
    if(__a_unlikely(ptrp != nullptr)){
        aExcSet(AEXC_repeat_write);
        return;
    }

    aExcClean();
    afs_add_fd(fd, type, AString_new(name), mod);
    if(aExcOccur()){
        return;
    }
    ptrp = afs_find(AString_new(name));
    if(__a_unlikely(ptrp == nullptr)){
        return;
    }

    auto ptr = *ptrp;
    file->fd = fd;

    aExcClean();
    file->name = A_INIT(AString); file->name.f->addBack(&file->name, AString_new(name));
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

    int ret = pread(fd, target, size, offset);

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

    int ret = pwrite(fd, source, size, offset);

    if(!((node->mod & __afmod_exclusive) && (node->tid == thrd_current()))){
        if(AFileNode_unlock_w(node) != 0){
            aExcSet(AEXC_system_error);
            return ret;
        }
    }

    return ret;
}


/*************************************************************************/
static inline AString AFile_getip(AFile* self){
    AString addr = A_INIT(AString);
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return addr;
    }

    if(!Afd_exist(self->fd)){
        aExcSet(AEXC_outdomain);
        return addr;
    }

    auto node = self->node.p;
    if(node->type != __aftype_socket){
        aExcSet(AEXC_outdomain);
        return addr;
    }
    auto name = &node->name; auto p = name->s;
    if(strncmp(p, "tcp|", 4) == 0){
        p += 4;
    }else if(strncmp(p, "udp|", 4) == 0){
        p += 4;
    }else if(strncmp(p, "raw|", 4) == 0){
        p += 4;
    }else if(strncmp(p, "unix|", 5) == 0){
        p += 5;
    }else{
        aExcSet(AEXC_outdomain);
        return addr;
    }

    if(strncmp(p, "server|", 7) == 0){
        p += 7;
    }else if(strncmp(p, "client|", 7) == 0){
        p += 7;
    }else{
        aExcSet(AEXC_outdomain);
        return addr;
    }

    addr.f->addBack(&addr, AString_new(p));
    return addr;
}

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
    A_DEST(AString, self->name);
    memset(self, 0, sizeof(AFile));
    self->fd = A_INIT(Afd);
}

void AFile_register(AFile* self, Afd fd, int type, const char* name, int mod){
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

void AFile_open(AFile* self, int type, const char* name, int mod){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(Afd_exist(self->fd) || name == nullptr || mod == 0 || type == 0){
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
    RAII(AString) path = __af_path_absolute(name);

    int mod = __afmod_read;
    AFile_open(&file, __aftype_file, path.s, mod);
    return file;
}
AFile aFileOutOpen(const char* name){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AString) path = __af_path_absolute(name);

    int mod = __afmod_write | __afmod_exclusive | __afmod_creat | __afmod_truncate;
    AFile_open(&file, __aftype_file, path.s, mod);
    return file;
}
AFile aFileEndOpen(const char* name){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AString) path = __af_path_absolute(name);

    int mod = __afmod_write | __afmod_appent | __afmod_creat;
    AFile_open(&file, __aftype_file, path.s, mod);
    return file;
}

AFile aDevInOpen(const char* name, bool noblock){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AString) path = __af_path_absolute(name);

    int mod = __afmod_read;
    if(noblock) mod |= __afmod_noblock;
    AFile_open(&file, __aftype_device, path.s, mod);
    return file;
}
AFile aDevOutOpen(const char* name, bool exclusive){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AString) path = __af_path_absolute(name);

    int mod = __afmod_write;
    if(exclusive) mod |= __afmod_exclusive;
    AFile_open(&file, __aftype_device, path.s, mod);
    return file;
}
AFile aDevInOutOpen(const char* name, bool noblock, bool exclusive){
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    RAII(AString) path = __af_path_absolute(name);

    int mod = __afmod_read | __afmod_write;
    if(noblock) mod |= __afmod_noblock;
    if(exclusive) mod |= __afmod_exclusive;
    AFile_open(&file, __aftype_device, path.s, mod);
    return file;
}

AFile aSocketTcpServerOpen(const char* name){
    RAII(AString) addr = AString_new("tcp|server|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketUdpServerOpen(const char* name){
    RAII(AString) addr = AString_new("udp|server|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketUnixServerOpen(const char* name){
    RAII(AString) addr = AString_new("unix|server|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketRawServerOpen(const char* name){
    RAII(AString) addr = AString_new("raw|server|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketTcpClientOpen(const char* name){
    RAII(AString) addr = AString_new("tcp|client|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketUdpClientOpen(const char* name){
    RAII(AString) addr = AString_new("udp|client|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketUnixClientOpen(const char* name){
    RAII(AString) addr = AString_new("unix|client|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketRawClientOpen(const char* name){
    RAII(AString) addr = AString_new("raw|client|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(name == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, AString_new(name)); if(aExcOccur()){
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    AFile_open(&file, __aftype_socket, addr.s, mod);
    return file;
}
AFile aSocketTcpAccept(AFile* tcp_server){
    static atomic_ulong accept_num = 0;

    RAII(AString) addr = AString_new("tcp|client|");
    AFile file = A_INIT(AFile);
    if(__a_unlikely(tcp_server == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }

    aExcClean(); RAII(AString) ip = AFile_getip(tcp_server); if(aExcOccur()){
        return file;
    }
    aExcClean(); addr.f->addBack(&addr, ip); if(aExcOccur()){
        return file;
    }
    char accept_end[12]; memset(accept_end, 0, 12);
    snprintf(accept_end, 10, "|%-8lu", (unsigned long)atomic_fetch_add(&accept_num, 1));
    aExcClean(); addr.f->addBack(&addr, AString_new(accept_end)); if(aExcOccur()){
        return file;
    }

    struct sockaddr_in client_addr; memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_len = sizeof(client_addr);
    Afd client_fd = accept(tcp_server->fd, (struct sockaddr*)&client_addr, &client_len);
    if(!Afd_exist(client_fd)){
        aExcSet(AEXC_system_error);
        return file;
    }

    int mod = __afmod_read | __afmod_write;
    aExcClean();
    AFile_register(&file, client_fd, __aftype_socket, addr.s, mod);
    if(aExcOccur()){
        a_close(client_fd);
    }
    return file;
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

    info.st_dev = (uint64_t)st.st_dev;
    info.st_ino = (uint64_t)st.st_ino;
    info.st_mode = (uint32_t)st.st_mode;
    info.st_nlink = (uint32_t)st.st_nlink;
    info.st_uid = (uint32_t)st.st_uid;
    info.st_gid = (uint32_t)st.st_gid;
    info.st_rdev = (uint64_t)st.st_rdev;
    info.st_size = (uint64_t)st.st_size;
    info.st_atime = (stat_time_t)st.st_atim.tv_sec;
    info.st_mtime = (stat_time_t)st.st_mtim.tv_sec;
    info.st_ctime = (stat_time_t)st.st_ctim.tv_sec;
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

    RAII(AString) all = A_INIT(AString);
    if(name[0] == '/'){
        aExcClean();
        all.f->addBack(&all, AString_new(name));
        if(aExcOccur()){
            return;
        }
    }else{
        aExcClean();
        RAII(AString) text = __af_path_absolute(".");
        all.f->addBack(&all, text);
        all.f->addBack(&all, AString_new("/"));
        all.f->addBack(&all, AString_new(name));
        if(aExcOccur()){
            return;
        }
    }

    aExcClean(); RAII(AString) path = __af_dir_extract(all.s); if(aExcOccur()){
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
        RAII(ALine(AString)) list = __af_ls(name);
        forEach(it, list){
            AString* x = it.p;
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

    RAII(AString) src = __af_path_absolute(name);
    RAII(AString) tar = A_INIT(AString);

    if(__af_isdir(target)){
        aExcClean();
        RAII(AString) fn = __af_name_extract(name);
        RAII(AString) pt = __af_path_absolute(target);

        tar.f->addBack(&tar, pt);
        int len = strlen(tar.s);
        if(len > 1 && tar.s[len - 1] != '/'){
            tar.f->addBack(&tar, AString_new("/"));
        }
        tar.f->addBack(&tar, fn);
        len = strlen(tar.s);
        if(len > 1 && tar.s[len - 1] == '/'){
            tar.f->popBack(&tar);
        }

        if(aExcOccur()){
            return;
        }
    }else if(target[0] == '/'){
        //绝对路径
        aExcClean();
        tar.f->addBack(&tar, AString_new(target));
        if(aExcOccur()){
            return;
        }
    }else{
        //文件名
        aExcClean();
        RAII(AString) pt = __af_dir_extract(src.s);

        tar.f->addBack(&tar, pt);
        int len = strlen(tar.s);
        if(len > 1 && tar.s[len - 1] != '/'){
            tar.f->addBack(&tar, AString_new("/"));
        }
        tar.f->addBack(&tar, AString_new(target));
        len = strlen(tar.s);
        if(len > 1 && tar.s[len - 1] == '/'){
            tar.f->popBack(&tar);
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
    RAII(AString) tar = AString_new(target);
    {
        int len = strlen(tar.s);
        if(len > 1 && tar.s[len - 1] != '/'){
            tar.f->addBack(&tar, AString_new("/"));
        }
    }
    if(__af_path_exist(target)){
        RAII(AString) src = __af_name_extract(name);
        tar.f->addBack(&tar, src);
        int len = strlen(tar.s);
        if(len > 1 && tar.s[len - 1] != '/'){
            tar.f->addBack(&tar, AString_new("/"));
        }
    }
    __af_mkdir(tar.s);
    RAII(ALine(AString)) list = __af_ls(name);
    if(aExcOccur()){
        return;
    }

    forEach(it, list){
        AString* entry = it.p;
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
    RAII(AString) tar = AString_new(new_name);
    if(__af_isdir(new_name)){
        aExcClean();
        RAII(AString) src = __af_name_extract(name);
        int len = strlen(tar.s);
        if(len > 1 && tar.s[len - 1] != '/'){
            tar.f->addBack(&tar, AString_new("/"));
        }
        tar.f->addBack(&tar, src);
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
        RAII(ALine(AString)) list = __af_ls(name);
        forEach(it, list){
            AString* x = it.p;
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

static ALine(AString) __af_ls(const char* name){
    ALine(AString) list = A_INIT(ALine(AString));

    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return list;
    }
    if(__af_isfile(name)){
        list.f->pushBack(&list, AString_new(name));
        return list;
    }
    if(!__af_isdir(name)){
        aExcSet(AEXC_outdomain);
        return list;
    }

    aExcClean(); RAII(AString) path = __af_path_absolute(name); if(aExcOccur()){
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
        RAII(AString) text = A_COPY(AString, path);
        text.f->addBack(&text, AString_new("/"));
        text.f->addBack(&text, AString_new(entry->d_name));
        if(aExcOccur()){
            break;
        }

        aExcClean();
        int len = strlen(text.s);
        if(len > 1 && text.s[len - 1] == '/'){
            text.f->popBack(&text);
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

static AString __af_path_absolute(const char* name){
    char buf[PATH_MAX]; memset(buf, 0, PATH_MAX);
    AString text = A_INIT(AString);
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return text;
    }

    aExcClean();
    if(name[0] == '/'){
        text.f->addBack(&text, AString_new(name));
    }else{
        if(realpath(".", buf) == nullptr){
            aExcSet(AEXC_system_error);
            return text;
        }

        text.f->addBack(&text, AString_new(buf));
        int len = strlen(text.s);
        if(len > 1 && text.s[len - 1] != '/'){
            text.f->addBack(&text, AString_new("/"));
        }
        text.f->addBack(&text, AString_new(name));
    }

    if(__af_isdir(text.s)){
        int len = strlen(text.s);
        if(len > 1 && text.s[len - 1] == '/'){
            text.f->popBack(&text);
        }
    }

    if(aExcOccur()){
        return A_INIT(AString);
    }

    return text;
}

static AString __af_name_extract(const char* name){
    AString text = A_INIT(AString);
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return text;
    }
    if(strcmp(name, "/") == 0){
        text.f->addBack(&text, AString_new(name));
        return text;
    }

    if(strlen(name) == 0){
        aExcSet(AEXC_outdomain);
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
    aExcClean(); text.f->addBack(&text, AString_new(p)); if(aExcOccur()){
        return text;
    }

    int len = strlen(text.s);
    if(len > 1 && text.s[len - 1] == '/'){
        text.f->popBack(&text);
    }

    return text;
}

static AString __af_dir_extract(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return A_INIT(AString);
    }
    if(strcmp(name, "/") == 0){
        AString path = A_INIT(AString);
        path.f->addBack(&path, AString_new(name));
        return path;
    }

    AString path = A_INIT(AString);
    if(name[0] == '/'){
        aExcClean();
        path.f->addBack(&path, AString_new(name));
        if(aExcOccur()){
            return A_INIT(AString);
        }
    }else{
        aExcClean();
        RAII(AString) all = __af_path_absolute(name);
        path.f->addBack(&path, all);
        if(aExcOccur()){
            return A_INIT(AString);
        }
    }

    char* p = path.s + path.f->getNumber(&path); p--;
    if(*p == '/') p++;
    while(p != path.s){
        if(*p == '/' && *(p-1) != '\\'){
            break;
        }
        p--;
    }
    path.f->truncate(&path, (uint32_t)(p - path.s));

    int len = strlen(path.s);
    if(len > 1 && path.s[len - 1] == '/'){
        path.f->popBack(&path);
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

AString af_dir_extract(const char* name){
    return __af_dir_extract(name);
}

AString af_name_extract(const char* name){
    return __af_name_extract(name);
}

AString af_path_absolute(const char* name){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return A_INIT(AString);
    }
    return __af_path_absolute(name);
}

ALine(AString) af_ls(const char* dir){
    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&afslock); if(aExcOccur()){
        return A_INIT(ALine(AString));
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
