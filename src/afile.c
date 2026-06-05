/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <afile.h>
#include <ahash.h>
#include <alock.h>

#if defined(__C_POSIX__)
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef st_atime
#undef st_atime
#endif
#ifdef st_mtime
#undef st_mtime
#endif
#ifdef st_ctime
#undef st_ctime
#endif

static inline AText __af_text_owned(const char* s){
    AText text = A_INIT(AText);
    if(s != nullptr){
        text.f->addBack(&text, AText_new(s));
    }
    return text;
}

static inline AText __af_join_path(const char* dir, const char* name){
    AText path = __af_text_owned(dir != nullptr && dir[0] != '\0' ? dir : ".");
    if(name == nullptr || name[0] == '\0'){
        return path;
    }
    if(path.byte_num != 0 && path.s[path.byte_num - 1] != '/'){
        path.f->pushBack(&path, Achar_new("/"));
    }
    path.f->addBack(&path, AText_new(name));
    return path;
}

static inline int __af_copy_file(const char* name, const char* target){
    int in = open(name, O_RDONLY);
    if(in < 0){
        return -1;
    }

    int out = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(out < 0){
        close(in);
        return -1;
    }

    char buf[8192];
    int ret = 0;
    for(;;){
        ssize_t n = read(in, buf, sizeof(buf));
        if(n == 0){
            break;
        }
        if(n < 0){
            ret = -1;
            break;
        }
        ssize_t off = 0;
        while(off < n){
            ssize_t w = write(out, buf + off, (size_t)(n - off));
            if(w < 0){
                ret = -1;
                break;
            }
            off += w;
        }
        if(ret != 0){
            break;
        }
    }

    if(close(in) != 0){
        ret = -1;
    }
    if(close(out) != 0){
        ret = -1;
    }
    return ret;
}

static inline void __af_cp_r_impl(const char* name, const char* target){
    struct stat st;
    if(lstat(name, &st) != 0){
        aExcSet(AEXC_system_error);
        return;
    }

    if(S_ISDIR(st.st_mode)){
        if(mkdir(target, st.st_mode & 0777) != 0 && errno != EEXIST){
            aExcSet(AEXC_system_error);
            return;
        }

        DIR* dp = opendir(name);
        if(dp == nullptr){
            aExcSet(AEXC_system_error);
            return;
        }

        struct dirent* ent;
        while((ent = readdir(dp)) != nullptr){
            if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
                continue;
            }
            RAII(AText) src = __af_join_path(name, ent->d_name);
            RAII(AText) dst = __af_join_path(target, ent->d_name);
            if(aExcOccur()){
                break;
            }
            __af_cp_r_impl(src.s, dst.s);
            if(aExcOccur()){
                break;
            }
        }
        if(closedir(dp) != 0 && !aExcOccur()){
            aExcSet(AEXC_system_error);
        }
        return;
    }

    if(S_ISREG(st.st_mode)){
        if(__af_copy_file(name, target) != 0){
            aExcSet(AEXC_system_error);
            return;
        }
        chmod(target, st.st_mode & 0777);
        return;
    }

    aExcSet(AEXC_invalid_function);
}

static inline void __af_rm_r_impl(const char* name){
    struct stat st;
    if(lstat(name, &st) != 0){
        aExcSet(AEXC_system_error);
        return;
    }

    if(S_ISDIR(st.st_mode)){
        DIR* dp = opendir(name);
        if(dp == nullptr){
            aExcSet(AEXC_system_error);
            return;
        }

        struct dirent* ent;
        while((ent = readdir(dp)) != nullptr){
            if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
                continue;
            }
            RAII(AText) child = __af_join_path(name, ent->d_name);
            if(aExcOccur()){
                break;
            }
            __af_rm_r_impl(child.s);
            if(aExcOccur()){
                break;
            }
        }
        if(closedir(dp) != 0 && !aExcOccur()){
            aExcSet(AEXC_system_error);
        }
        if(aExcOccur()){
            return;
        }
        if(rmdir(name) != 0){
            aExcSet(AEXC_system_error);
        }
        return;
    }

    if(unlink(name) != 0){
        aExcSet(AEXC_system_error);
    }
}

static inline void __af_chmod_r_impl(const char* name, char p){
    af_chmod(name, p);
    if(aExcOccur() || !af_isdir(name)){
        return;
    }

    DIR* dp = opendir(name);
    if(dp == nullptr){
        aExcSet(AEXC_system_error);
        return;
    }

    struct dirent* ent;
    while((ent = readdir(dp)) != nullptr){
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
            continue;
        }
        RAII(AText) child = __af_join_path(name, ent->d_name);
        if(aExcOccur()){
            break;
        }
        __af_chmod_r_impl(child.s, p);
        if(aExcOccur()){
            break;
        }
    }
    if(closedir(dp) != 0 && !aExcOccur()){
        aExcSet(AEXC_system_error);
    }
}

/* 创建目录 */
void af_mkdir(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(mkdir(name, 0755) != 0 && errno != EEXIST){
        aExcSet(AEXC_system_error);
    }
}
void af_mkdir_p(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(name[0] == '\0'){
        aExcSet(AEXC_outdomain);
        return;
    }

    RAII(AText) path = __af_text_owned(name);
    if(aExcOccur()){
        return;
    }

    for(uint32_t i = 1; i < path.byte_num; ++i){
        if(path.s[i] != '/'){
            continue;
        }
        path.s[i] = '\0';
        if(path.s[0] != '\0' && mkdir(path.s, 0755) != 0 && errno != EEXIST){
            path.s[i] = '/';
            aExcSet(AEXC_system_error);
            return;
        }
        path.s[i] = '/';
    }
    if(mkdir(path.s, 0755) != 0 && errno != EEXIST){
        aExcSet(AEXC_system_error);
    }
}

/* 删除文件或目录 */
void af_rm(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(remove(name) != 0){
        aExcSet(AEXC_system_error);
    }
}
void af_rm_r(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    __af_rm_r_impl(name);
}

/* 复制文件或目录 */
void af_cp(const char* name, const char* target){
    if(name == nullptr || target == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__af_copy_file(name, target) != 0){
        aExcSet(AEXC_system_error);
    }
}
void af_cp_r(const char* name, const char* target){
    if(name == nullptr || target == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    __af_cp_r_impl(name, target);
}

/* 移动文件或目录 */
void af_mv(const char* name, const char* new_name){
    if(name == nullptr || new_name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(rename(name, new_name) != 0){
        aExcSet(AEXC_system_error);
    }
}

/* 创建空文件 */
void af_touch(const char* name){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    int fd = open(name, O_WRONLY | O_CREAT, 0644);
    if(fd < 0){
        aExcSet(AEXC_system_error);
        return;
    }
    if(close(fd) != 0){
        aExcSet(AEXC_system_error);
    }
}

/* 修改当前用户文件权限 */ /* p=0|1|2|4 */
void af_chmod(const char* name, char p){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    struct stat st;
    if(stat(name, &st) != 0){
        aExcSet(AEXC_system_error);
        return;
    }
    mode_t mode = st.st_mode & ~(S_IRUSR | S_IWUSR | S_IXUSR);
    if(p & 4) mode |= S_IRUSR;
    if(p & 2) mode |= S_IWUSR;
    if(p & 1) mode |= S_IXUSR;
    if(chmod(name, mode) != 0){
        aExcSet(AEXC_system_error);
    }
}
void af_chmod_r(const char* name, char p){
    if(name == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    __af_chmod_r_impl(name, p);
}

/* 目标是文件 */
bool af_isfile(const char* name){
    struct stat st;
    return name != nullptr && stat(name, &st) == 0 && S_ISREG(st.st_mode);
}
/* 目标是目录 */
bool af_isdir(const char* name){
    struct stat st;
    return name != nullptr && stat(name, &st) == 0 && S_ISDIR(st.st_mode);
}
/* 目标是设备 */
bool af_isdev(const char* name){
    struct stat st;
    return name != nullptr && stat(name, &st) == 0 &&
           (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode));
}

/* 提取目录 */
/* 若为目录则直接返回 */
/* 若为空则返回当前目录"." */
AText af_dir_extract(const char* name){
    if(name == nullptr || name[0] == '\0'){
        return __af_text_owned(".");
    }
    if(af_isdir(name)){
        return __af_text_owned(name);
    }

    const char* slash = strrchr(name, '/');
    if(slash == nullptr){
        return __af_text_owned(".");
    }
    if(slash == name){
        return __af_text_owned("/");
    }

    AText dir = A_INIT(AText);
    AText src = AText_new(name);
    dir.f->addBack(&dir, src);
    if(!aExcOccur()){
        dir.f->truncate(&dir, (uint32_t)(slash - name));
    }
    return dir;
}

/* 获取绝对路径 */
/* 若为空则返回当前目录的绝对路径 */
AText af_path_absolute(const char* name){
    const char* src = (name != nullptr && name[0] != '\0') ? name : ".";

    char* resolved = realpath(src, nullptr);
    if(resolved != nullptr){
        AText out = __af_text_owned(resolved);
        free(resolved);
        return out;
    }

    if(src[0] == '/'){
        return __af_text_owned(src);
    }

    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) == nullptr){
        aExcSet(AEXC_system_error);
        return A_INIT(AText);
    }
    return __af_join_path(cwd, src);
}

/* ls */
static inline ALine(AText) __af_ls(const char* dir, int mode){
    ALine(AText) list = A_INIT(ALine(AText));
    const char* path = (dir != nullptr && dir[0] != '\0') ? dir : ".";
    DIR* dp = opendir(path);
    if(dp == nullptr){
        aExcSet(AEXC_system_error);
        return list;
    }

    struct dirent* ent;
    while((ent = readdir(dp)) != nullptr){
        bool dot = strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0;
        bool hidden = ent->d_name[0] == '.';
        if(mode == 0 && hidden){
            continue;
        }
        if(mode == 2 && dot){
            continue;
        }

        RAII(AText) item = __af_text_owned(ent->d_name);
        if(aExcOccur()){
            break;
        }
        list.f->pushBack(&list, item);
        if(aExcOccur()){
            break;
        }
    }

    if(closedir(dp) != 0 && !aExcOccur()){
        aExcSet(AEXC_system_error);
    }
    return list;
}
ALine(AText) af_ls(const char* dir){
    return __af_ls(dir, 0);
}
ALine(AText) af_ls_a(const char* dir){
    return __af_ls(dir, 1);
}
ALine(AText) af_ls_A(const char* dir){
    return __af_ls(dir, 2);
}

/* file info */
AFileInfo af_get_info(const char* name){
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
    pid_t pid = getpid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false; self->rnum = 0;
    }
    if(!self->fl_stat && self->rnum == 0 && !self->exclusive){
        self->fl.l_type = F_RDLCK;
        if(fcntl(self->fd, F_SETLKW, &self->fl) == -1){
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
    pid_t pid = getpid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false;
    }
    if(!self->fl_stat && !self->exclusive){
        self->fl.l_type = F_WRLCK;
        if(fcntl(self->fd, F_SETLKW, &self->fl) == -1){
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
    pid_t pid = getpid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false; self->rnum = 0;
    }
    if(self->rnum > 0) self->rnum--;
    if(self->fl_stat && self->rnum == 0 && !self->exclusive){
        self->fl.l_type = F_UNLCK;
        fcntl(self->fd, F_SETLKW, &self->fl);
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
    pid_t pid = getpid();
    if(pid != self->pid){
        self->pid = pid; self->fl_stat = false;
    }
    if(self->fl_stat && !self->exclusive){
        self->fl.l_type = F_UNLCK;
        fcntl(self->fd, F_SETLKW, &self->fl);
        self->fl_stat = false;
    }
    AMtx_unlock(&self->fl_lock);

    AMtxRW_unlock_write(&self->lock);
    return 0;
}

static void AFileNode_open(AFileNode* self, AText name, int mod, bool noblock, bool exclusive){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    self->name = A_COPY(AText, name);
    self->mod = mod;
    if(af_isdev(name.s) && noblock){
        self->noblock = noblock;
    }
    if(exclusive && self->mod != __afmod_r){
        self->exclusive = true;
    }
    if(af_isfile(name.s) && !af_isdev(name.s) && self->mod == __afmod_w){
        self->exclusive = true;
    }

    int flag = 0;
    switch(self->mod){
        case __afmod_r: flag = O_RDONLY; break;
        case __afmod_w: flag = O_WRONLY; break;
        case __afmod_rw: flag = O_RDWR; break;
        case __afmod_aw: flag = O_WRONLY; break;
        default:{
            A_DEST(AFileNode, *self);
            aExcSet(AEXC_outdomain);
            return;
        }
    }

    if(!af_isdev(name.s) && self->mod != __afmod_r){
        flag |= O_CREAT;
        if(self->mod == __afmod_w){
            flag |= O_TRUNC;
        }
    }

    if(self->noblock){
        flag |= O_NONBLOCK;
    }

    if(self->mod == __afmod_aw){
        flag |= O_APPEND;
    }

    if(!af_isdev(name.s)){
        self->fd = open(name.s, flag, 0644);
    }else{
        self->fd = open(name.s, flag);
    }

    if(self->fd < 0){
        aExcSet(AEXC_system_error);
        A_DEST(AFileNode, *self);
        return;
    }

    if(self->exclusive){
        self->fl.l_type = F_WRLCK;
        if(fcntl(self->fd, F_SETLKW, &self->fl) == -1){
            A_DEST(AFileNode, *self);
            aExcSet(AEXC_system_error);
            return;
        }
        self->fl_stat = true;
    }
}

static uint32_t AFileNode_close(AFileNode* self, int fd){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 1;
    }
    if(fd < 0){
        aExcSet(AEXC_system_error);
        return -1;
    }

    if(self->exclusive || (self->fl_stat && self->num > 0)){
        auto ls = &self->fd_list;
        ls->f->pushBack(ls, fd);
    }else{
        close(fd);
    }

    if(self->num > 0){
        self->num--;
    }

    return self->num;
}

static int AFileNode_instantiation(AFileNode* self, int mod, bool noblock){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return -1;
    }

    if(mod != self->mod || noblock != self->noblock){
        aExcSet(AEXC_system_error);
        return -1;
    }

    if(self->num > 0 && self->exclusive){
        aExcSet(AEXC_system_error);
        return -1;
    }

    int flag = 0;
    switch(self->mod){
        case __afmod_r: flag = O_RDONLY; break;
        case __afmod_w: flag = O_WRONLY; break;
        case __afmod_rw: flag = O_RDWR; break;
        case __afmod_aw: flag = O_WRONLY; break;
        default:{
            aExcSet(AEXC_outdomain);
            return -1;
        }
    }

    auto name = self->name;
    if(self->noblock){
        flag |= O_NONBLOCK;
    }

    if(self->mod == __afmod_aw){
        flag |= O_APPEND;
    }

    int fd = open(name.s, flag);
    if(fd < 0){
        aExcSet(AEXC_system_error);
        return -1;
    }

    self->num++;
    return fd;
}



/*************************************************************************/
AHash_Define(AText,AShPtr(AFileNode));
AHash_Generate(AText,AShPtr(AFileNode));
A_TYPE_REGISTER(AHash(AText,AShPtr(AFileNode)));

typedef struct{
    bool                            flag;
    AMtxRW                          lock;
    AHash(AText,AShPtr(AFileNode))  table;
}AFileSystem;

static inline void A_OBJ_INIT(AFileSystem)(AFileSystem* self){
    aExcClean(); self->lock = A_INIT(AMtxRW); if(aExcOccur()){
        return;
    }

    aExcClean(); self->table = A_INIT(AHash(AText,AShPtr(AFileNode))); if(aExcOccur()){
        return;
    }

    self->flag = true;
}

static inline void A_OBJ_DEST(AFileSystem)(AFileSystem* self){
    self->flag = false;
    A_DEST(AMtxRW, self->lock);
    A_DEST(AHash(AText,AShPtr(AFileNode)), self->table);
}

static inline void A_OBJ_COPY(AFileSystem)(__noused AFileSystem* self, __noused const AFileSystem* that){
    aExcSet(AEXC_init_failed);
}

static inline int A_OBJ_CMPD(AFileSystem)(__noused const AFileSystem* self, __noused const AFileSystem* that){
    return 0;
}

A_TYPE_REGISTER(AFileSystem);

static inline void AFileSystem_add(AFileSystem* self, AText name, int mod, bool noblock, bool exclusive){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    auto tab = &self->table;

    aExcClean(); RAII(AShPtr(AFileNode)) ptr = AShPtrNew(AFileNode);if(aExcOccur()){
        return;
    }
    AFileNode* node = ptr.p;

    AFileNode_open(node, name, mod, noblock, exclusive);if(aExcOccur()){
        return;
    }

    tab->f->ins(tab, name, ptr);
}

static inline AShPtr(AFileNode)* AFileSystem_find(AFileSystem* self, AText name){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return nullptr;
    }

    auto tab = &self->table;
    return tab->f->at(tab, name);
}

static inline void AFileSystem_rm(AFileSystem* self, AText name){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    auto tab = &self->table;
    tab->f->rm(tab, name);
}

static inline void AFileSystem_close(AFileSystem* self, AText name, int fd){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&self->lock); if(aExcOccur()){
        return;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return;
    }

    auto ptrp = AFileSystem_find(self, name);
    if(__a_unlikely(ptrp == nullptr)){
        return;
    }
    auto ptr = *ptrp;
    if(AFileNode_close(ptr.p, fd) == 0){
        AFileSystem_rm(self, name);
    }
}

static inline AFile AFileSystem_open(AFileSystem* self, int mod, bool noblock, bool exclusive, AText name){
    AFile file = A_INIT(AFile);

    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&self->lock); if(aExcOccur()){
        return file;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return file;
    }

    auto ptrp = AFileSystem_find(self, name);
    if(__a_unlikely(ptrp == nullptr)){
        aExcClean();AFileSystem_add(self, name, mod, noblock, exclusive);if(aExcOccur()){
            return file;
        }
        ptrp = AFileSystem_find(self, name);
        if(__a_unlikely(ptrp == nullptr)){
            return file;
        }
    }
    auto ptr = *ptrp;
    auto node = ptr.p;
    aExcClean(); file.fd = AFileNode_instantiation(node, mod, noblock); if(aExcOccur()){
        return file;
    }
    file.name = A_COPY(AText, name);
    file.node = A_COPY(AShPtr(AFileNode), ptr);
    return file;
}

static inline AFile AFileSystem_open_copy(AFileSystem* self, AText name){
    AFile file = A_INIT(AFile);

    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return file;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_wlock(&self->lock); if(aExcOccur()){
        return file;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return file;
    }

    auto ptrp = AFileSystem_find(self, name);
    if(__a_unlikely(ptrp == nullptr)){
        aExcSet(AEXC_outdomain);
        return file;
    }
    auto ptr = *ptrp;
    auto node = ptr.p;
    aExcClean(); file.fd = AFileNode_instantiation(node, node->mod, node->noblock); if(aExcOccur()){
        return file;
    }
    file.name = A_COPY(AText, name);
    file.node = A_COPY(AShPtr(AFileNode), ptr);
    return file;
}

static inline int AFileSystem_ioctl(AFileSystem* self, AShPtr(AFileNode) ptr, int fd, int cmd, void* buf){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || fd < 0)){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    if(!(node->mod == __afmod_rw)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&self->lock); if(aExcOccur()){
        return 0;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    A_DEST(AAutoKey, key);

    if(AFileNode_uplock_w(node) != 0){
        aExcSet(AEXC_system_error);
        return 0;
    }

    int ret = ioctl(fd, cmd, buf);

    if(AFileNode_unlock_w(node) != 0){
        aExcSet(AEXC_system_error);
        return ret;
    }

    return ret;
}

static inline int AFileSystem_read(AFileSystem* self, AShPtr(AFileNode) ptr, int fd, void* target, size_t size){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || fd < 0)){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    if(!(node->mod == __afmod_r || node->mod == __afmod_rw)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&self->lock); if(aExcOccur()){
        return 0;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    A_DEST(AAutoKey, key);

    if(AFileNode_uplock_r(node) != 0){
        aExcSet(AEXC_system_error);
        return 0;
    }

    int ret = read(fd, target, size);

    if(AFileNode_unlock_r(node) != 0){
        aExcSet(AEXC_system_error);
        return ret;
    }

    return ret;
}

static inline int AFileSystem_read_pos(AFileSystem* self, AShPtr(AFileNode) ptr, int fd, uint64_t offset, void* target, size_t size){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || fd < 0)){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    if(!(node->mod == __afmod_r || node->mod == __afmod_rw)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&self->lock); if(aExcOccur()){
        return 0;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    A_DEST(AAutoKey, key);

    if(AFileNode_uplock_r(node) != 0){
        aExcSet(AEXC_system_error);
        return 0;
    }

    int ret = pread(fd, target, size, offset);

   if(AFileNode_unlock_r(node) != 0){
        aExcSet(AEXC_system_error);
        return ret;
    }


    return ret;
}

static inline int AFileSystem_write(AFileSystem* self, AShPtr(AFileNode) ptr, int fd, void* source, size_t size){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || fd < 0)){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    if((node->mod == __afmod_r)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&self->lock); if(aExcOccur()){
        return 0;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    A_DEST(AAutoKey, key);

    if(AFileNode_uplock_w(node) != 0){
        aExcSet(AEXC_system_error);
        return 0;
    }

    int ret = write(fd, source, size);

    if(AFileNode_unlock_w(node) != 0){
        aExcSet(AEXC_system_error);
        return ret;
    }

    return ret;
}

static inline int AFileSystem_write_pos(AFileSystem* self, AShPtr(AFileNode) ptr, int fd, uint64_t offset, void* source, size_t size){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    auto node = ptr.p;
    if(__a_unlikely(node == nullptr || fd < 0)){
        aExcSet(AEXC_outdomain);
        return 0;
    }
    if((node->mod == __afmod_r)){
        aExcSet(AEXC_system_error);
        return 0;
    }

    aExcClean(); RAII(AAutoKey) key = AMtxRW_rlock(&self->lock); if(aExcOccur()){
        return 0;
    }
    if(!self->flag){
        aExcSet(AEXC_system_error);
        return 0;
    }
    A_DEST(AAutoKey, key);

    if(AFileNode_uplock_w(node) != 0){
        aExcSet(AEXC_system_error);
        return 0;
    }

    int ret = pwrite(fd, source, size, offset);

    if(AFileNode_unlock_w(node) != 0){
        aExcSet(AEXC_system_error);
        return ret;
    }

    return ret;
}

#endif /* posix */



/*************************************************************************/
static AFileSystem afilesystem;

__attribute__((constructor)) static inline void a_file_system_start(){
    afilesystem = A_INIT(AFileSystem);
}
__attribute__((destructor)) static inline void a_file_system_poweroff(){
    A_DEST(AFileSystem, afilesystem);
}

void AFile_close(AFile* self){
    if(self == nullptr || self->fd < 0){
        return;
    }
    aExcClean();
    AFileSystem_close(&afilesystem, self->name, self->fd);
    if(aExcOccur()){
        if(self->fd >= 0) close(self->fd);
    }

    A_DEST(AShPtr(AFileNode), self->node);
    A_DEST(AText, self->name);
    memset(self, 0, sizeof(AFile));
    self->fd = -1;
}
AFile AFile_open_copy(AText name){
    return AFileSystem_open_copy(&afilesystem, name);
}
AFile AFile_open(int mod, bool noblock, bool exclusive, AText name){
    return AFileSystem_open(&afilesystem, mod, noblock, exclusive, name);
}

int32_t  AFile_ioctl(AFile* self, int32_t cmd, void* buf){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    if(self->fd < 0){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = AFileSystem_ioctl(&afilesystem, self->node, self->fd, cmd, buf);
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
    if(self->fd < 0){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = AFileSystem_read(&afilesystem, self->node, self->fd, target, size);
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
    if(self->fd < 0){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = AFileSystem_write(&afilesystem, self->node, self->fd, source, size);
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
    if(self->fd < 0){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = AFileSystem_read_pos(&afilesystem, self->node, self->fd, offset, target, size);
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
    if(self->fd < 0){
        aExcSet(AEXC_outdomain);
        return 0;
    }

    int ret = AFileSystem_write_pos(&afilesystem, self->node, self->fd, offset, source, size);
    if(ret < 0){
        aExcSet(AEXC_system_error);
        ret = 0;
    }
    return (uint32_t)ret;
}



