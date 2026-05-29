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
__noused __weak __visibility(protected) void A_OBJ_INIT(AFile)(AFile* self){
    self->name = A_INIT(AText);
}
__noused __weak __visibility(protected) void A_OBJ_DEST(AFile)(AFile* self){
    if(self->fp != nullptr) fclose(self->fp);
    A_DEST(AText, self->name);
}
__noused __weak __visibility(protected) void A_OBJ_COPY(AFile)(AFile* self, const AFile* that){
    self->name = A_COPY(AText, that->name);
    if(AText_empty(&self->name)) self->name = AText_new("(null)");
}
__noused __weak __visibility(protected) int A_OBJ_CMPD(AFile)(const AFile* self, const AFile* that){
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
__noused __weak __visibility(protected) void A_OBJ_COPY(ARFile)(ARFile* self, const ARFile* that){
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
__noused __weak __visibility(protected) void A_OBJ_COPY(AWFile)(AWFile* self, __noused const AWFile* that){
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
__noused __weak __visibility(protected) void A_OBJ_COPY(APFile)(APFile* self, __noused const APFile* that){
    __AFile_open((AFile*)self, "a");
}
A_CLASS_REGISTER(APFile);



ARFile aReadFile(const char* name);
AWFile aWriteFile(const char* name);
APFile aAppendFile(const char* name);

ARFile aReadFileMem(const void* mem, uint64_t size);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__afile_h__*/

