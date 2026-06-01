/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <afile.h>

static void __AFile_setname(AFile* self, const char* name){
    if(name == nullptr) name = "(null)";
    self->name = AText_new(name);
}
static inline uint32_t __AFile_read(AFile* self, uint32_t size, void* target){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    FILE* fp = self->fp;
    if(__a_unlikely(fp == nullptr)){
        aExcSet(AEXC_file_noexist);
        return 0;
    }
    return (uint32_t)fread(target, size, 1, fp);
    return 0;
}
static inline uint32_t __AFile_write(AFile* self, uint32_t size, void* target){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    FILE* fp = self->fp;
    if(__a_unlikely(fp == nullptr)){
        aExcSet(AEXC_file_noexist);
        return 0;
    }
    return (uint32_t)fwrite(target, size, 1, fp);
    return 0;
}
#if defined(__C_POSIX__)
#include <sys/stat.h>
static inline uint64_t __AFile_size(AFile* self){
    struct stat st;
    if(stat(self->name.s, &st) != 0){
        return 0;  // 获取失败
    }
    return (uint64_t)st.st_size;
}
#endif /* posix */

#if defined(__C_WINDOWS__)
#include <windows.h>
static inline uint64_t __AFile_size(AFile* self){
    HANDLE hFile = CreateFileA(self->name.s, GENERIC_READ,
                               FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return 0;

    LARGE_INTEGER size;
    BOOL ok = GetFileSizeEx(hFile, &size);
    CloseHandle(hFile);
    if (!ok) return 0;
    return (uint64_t)size.QuadPart;
}
#endif /* windows */
void __AFile_open(AFile* self, const char* mode){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return;
    }
    self->fp = fopen(self->name.s, mode);
    if(self->fp == nullptr){
        aExcSet(AEXC_file_noexist);
        return;
    }
    self->size = __AFile_size(self);
}
static inline uint64_t __AFile_getsize(AFile* self){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    return self->size;
}



ARFile aFileOpen(const char* name){
    auto file = A_INIT(ARFile);
    __AFile_setname((AFile*)&file, name);
    __AFile_open((AFile*)&file, "r");
    return file;
}
AWFile aFileCreate(const char* name){
    auto file = A_INIT(AWFile);
    __AFile_setname((AFile*)&file, name);
    __AFile_open((AFile*)&file, "w");
    return file;
}
APFile aFileAppend(const char* name){
    auto file = A_INIT(APFile);
    __AFile_setname((AFile*)&file, name);
    __AFile_open((AFile*)&file, "a");
    return file;
}

ARFile aMemoryOpen(const void* mem, uint64_t size){
    const char* name = "(tmpfile)";
    auto file = A_INIT(ARFile);
    __AFile_setname((AFile*)&file, name);
    file.mem = mem, ((AFile*)&file)->size = size;
    return file;

}



uint64_t ARFile_size(ARFile* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    uint32_t size = __AFile_getsize((AFile*)self);
    if(size >= self->offset) return size - self->offset;
    return 0;
}
uint32_t ARFile_read(ARFile* self, uint32_t size, void* target){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    if(size == 0 || target == nullptr) return 0;
    uint64_t size_rem = ARFile_size(self);
    if(size_rem <= size) size = size_rem;

    if(self->mem == nullptr){
        uint32_t n = __AFile_read((AFile*)self, size, target);
        self->offset += n;
        return n;
    }else{
        memcpy(target, self->mem + self->offset, size);
        uint32_t n = size;
        self->offset += n;
        return n;
    }
}

uint64_t AWFile_size(AWFile* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    return self->addsize;
}
uint32_t AWFile_write(AWFile* self, uint32_t size, void* target){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    if(size == 0 || target == nullptr) return 0;

    uint32_t n = __AFile_write((AFile*)self, size, target);
    self->addsize += n;
    return n;
}

uint64_t APFile_size(APFile* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    return self->addsize + __AFile_getsize((AFile*)self);
}
uint32_t APFile_append(APFile* self, uint32_t size, void* target){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    if(size == 0 || target == nullptr) return 0;

    uint32_t n = __AFile_write((AFile*)self, size, target);
    self->addsize += n;
    return n;
}



#if defined(__C_POSIX__)
int32_t  ADev_ioctl(ADev* self, int32_t cmd, void* buf){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return aExcGet();
    }
    if(!self->stat){
        aExcSet(AEXC_system_error);
        return aExcGet();
    }
    return ioctl(self->fd, cmd, buf);
}
uint32_t ADev_read (ADev* self, uint32_t size, void* source){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return aExcGet();
    }
    if(!self->stat){
        aExcSet(AEXC_system_error);
        return aExcGet();
    }
    return read(self->fd, source, size);
}
uint32_t ADev_write(ADev* self, uint32_t size, void* target){
    if(self == nullptr){
        aExcSet(AEXC_nullptr);
        return aExcGet();
    }
    if(!self->stat){
        aExcSet(AEXC_system_error);
        return aExcGet();
    }
    return write(self->fd, target, size);
}

ADev aDevOpen(const char* name){
    ADev dev = A_INIT(ADev);
    dev.name = AText_new(name);
    dev.noblock = false;
    if(name == nullptr){
        aExcSet(AEXC_system_error);
    }else{
        dev.fd = open(name, O_RDWR);
        if(dev.fd < 0){
            aExcSet(AEXC_system_error);
        }else{
            dev.stat = true;
        }
    }
    return dev;
}

ADev aDevOpen_nb(const char* name){
    ADev dev = A_INIT(ADev);
    dev.name = AText_new(name);
    dev.noblock = true;
    if(name == nullptr){
        aExcSet(AEXC_system_error);
    }else{
        dev.fd = open(name, O_RDWR | O_NONBLOCK);
        if(dev.fd < 0){
            aExcSet(AEXC_system_error);
        }else{
            dev.stat = true;
        }
    }
    return dev;
}

#endif /* posix */

