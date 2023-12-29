#ifndef FILE_TYPE_H
#define FILE_TYPE_H
#include "mold.h"
typedef enum {
  UNKNOWN,
  EMPTY,
  ELF_OBJ,
  ELF_DSO,
  MACH_OBJ,
  MACH_EXE,
  MACH_DYLIB,
  MACH_BUNDLE,
  MACH_UNIVERSAL,
  AR,
  THIN_AR,
  TAPI,
  TEXT,
  GCC_LTO_OBJ,
  LLVM_BITCODE,
} FileType;

static inline int starts_with(const char* str, const char* prefix) {
    size_t prefixLen = strlen(prefix);
    return strncmp(str, prefix, prefixLen) == 0;
}

static inline FileType get_file_type (Context *ctx,MappedFile *mf) {
    StringView data = get_contents(mf);
    if (starts_with(data.data, "\177ELF")) {
        // printf("ELF file detected.\n");
        u8 byte_order = ((ElfEhdr *)data.data)->e_ident[EI_DATA];
        // printf("%d\n",*((ElfEhdr *)data.data)->e_type.val);

        if (byte_order == ELFDATA2LSB) {
            if (*((ElfEhdr *)data.data)->e_type.val == ET_REL) {
                if ( ((ElfEhdr *)data.data)->e_ident[EI_CLASS] == ELFCLASS32) {
                    return ELF_OBJ;
                }
            }
        }
    } else {
        printf("Not an ELF file.\n");
    }
    
}
#endif