// mold.h

#ifndef MOLD_H  // 头文件保护，防止重复包含
#define MOLD_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
typedef struct Context Context;
typedef struct StringView{
    const char* data;
    size_t size;
} StringView;

#include "uthash.h"
#include "new_vector.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "./elf.h"

struct OutputSectionKey {
  char *name;
  u64 type;
  u64 flags;
};
typedef struct OutputSectionKey OutputSectionKey;

#include "bigvector.h"
#include "uthash.h"
#include "config.h"
#include "symbol.h"
#include "mapped_file.h"
// 定义上下文信息的结构体
ARM32 target;



typedef struct {
    const char* key;
    void* value;
} SymbolEntry;

typedef struct ObjectFile ObjectFile;
typedef struct OutputS OutputSection;
// 输出段的结构体
typedef struct 
{ 
    vector symbols;
    i64 first_global;
    /* data */
    char *filename;
    bool is_dso;
    u32 priority;
    bool is_alive;
    StringView shstrtab;
    

    u64 local_symtab_idx;
    u64 global_symtab_idx;
    u64 num_local_symtab;
    u64 num_global_symtab;
    u64 strtab_offset;
    u64 strtab_size;

    ElfShdr **elf_sections;    
    int elf_sections_num;
    ElfSym **elf_syms;
    MappedFile *inputfile_mf;
    StringView symbol_strtab;
    vector local_syms;
    vector frag_syms;
} Inputefile;

typedef struct {
    /* 输入节的内容 */
    ObjectFile *file;
    OutputSection *output_section;
    u64 sh_size;
    StringView *contents;

    i32 fde_begin;
    i32 fde_end;

    u64 offset;
    u32 shndx;
    u32 relsec_idx;
    u32 reldyn_offset;
    bool uncompressed;

    u8 p2align;
    bool is_visited;
    bool is_alive;
    bool is_constucted;
} InputSection;

// ObjectFile 结构体
struct ObjectFile{
    Inputefile inputfile;
    char *archive_name;
    InputSection **sections;
    int num_sections;

    OutputSection **output_section;
    vector mergeable_sections;

    vector elf_sections2;

    bool is_in_lib;
    bool exclude_libs;
    bool is_lto_obj;
    bool needs_executable_stack;
    // ...
    u64 num_dynrel;
    u64 reldyn_offset;

    u64 fde_idx;
    u64 fde_offset;
    u64 fde_size;

    ElfShdr *symtab_sec;
    vector symbols;

    BitVector has_symver;
} ;

typedef struct {
    enum { NONE, SECTION, GROUP, ADDR, ALIGN, SYMBOL } type;
    char *name;
    u64 value;
} SectionOrder;

#include "output_file.h"
typedef enum {
    SEPARATE_LOADABLE_SEGMENTS,
    SEPARATE_CODE,
    NOSEPARATE_CODE,
} SeparateCodeKind;

struct Context{
    char** cmdline_args;
    int num_args;

    vector objs;
    vector string_pool;

    struct {
        /* data */
        Symbol *entry;
        Symbol *fini;
        Symbol *init;
        Symbol **undefined;
        bool color_diagnostics;
        char *output;
        char *emulation;
        bool fork;
        bool is_static;
        bool relocatable;
        int64_t thread_count;
        int undefined_count;
        bool oformat_binary;
        bool relocatable_merge_sections;
        bool gc_sections;
        bool eh_frame_hdr;
        vector section_order;

        SeparateCodeKind z_separate_code;
        bool z_defs;
        bool z_delete;
        bool z_dlopen;
        bool z_dump;
        bool z_dynamic_undefined_weak;
        bool z_execstack;
        bool z_execstack_if_needed;
        bool z_ibt;
        bool z_initfirst;
        bool z_interpose;
        bool z_keep_text_section_prefix;
        bool z_nodefaultlib;
        bool z_now;
        bool z_origin;
        bool z_relro;
        bool z_rewrite_endbr;
        bool z_sectionheader;
        bool z_shstk;
        bool z_text;
    } arg;

    bool is_static;
    bool in_lib;
    i64 file_priority;
    // Symbol table
    Symbol *symbol_map;
    int map_size;
    // 其他成员变量的定义
    ObjectFile *internal_obj;
    ObjectFile **obj_pool;
    vector osec_pool;
    vector internal_esyms;
    vector merged_sections;
    int merged_sections_count;
    // 里面放置一个个section
    vector chunks;

    OutputEhdr *ehdr;
    OutputShdr *shdr;
    OutputPhdr *phdr;
    GotSection *got;
    GotPltSection *gotplt;
    RelPltSection *relplt;
    RelDynSection *reldyn;
    // RelrDynSection *relrdyn;
    // DynamicSection *dynamic;
    StrtabSection *strtab;
    DynsymSection *dynsym;
    EhFrameSection *eh_frame;
    ShstrtabSection *shstrtab;
    EhFrameHdrSection *eh_frame_hdr;
    // DynstrSection *dynstr;
    // HashSection *hash;
    // ShstrtabSection *shstrtab;
    PltSection *plt;
    PltGotSection *pltgot;
    SymtabSection *symtab;
} ;

#include "filetype.h"
#include "input_file.h"
#include "cmdline.h"
#include "hashmap.h"
#include "elf_symbol.h"

static inline i64 get_shndx(ElfSym *esym) {
    if (*esym->st_shndx.val == SHN_XINDEX)
        return -1;
    return *esym->st_shndx.val;
}

char *mold_version;
void create_internal_file(Context *ctx);
void add_object_to_pool(ObjectFile **pool, int *size, ObjectFile *obj);
void resolve_symbols(Context *ctx);
extern MergedSection *get_instance(Context *ctx, char *name,
                               i64 type, i64 flags,
                               i64 entsize, i64 addralign);
void compute_merged_section_sizes(Context *ctx);
void create_synthetic_sections(Context *ctx);
void create_output_sections(Context *ctx);
void define_input_section(ObjectFile *file,Context *ctx,int i);
void input_sec_uncompress(Context *ctx,ObjectFile *file,int i);
void initialize_mergeable_sections(Context *ctx,ObjectFile *file);
void resolve_section_pieces(Context *ctx);
uint64_t hash_string(char *str);
SectionFragment *merge_sec_insert(Context *ctx, StringView *data, u64 hash,
                         i64 p2align,MergedSection *sec);
void file_resolve_section_pieces(Context *ctx,ObjectFile *file);
void assign_offsets(Context *ctx,MergedSection *sec);
// 获取输入input_section
static inline ElfShdr *get_shdr(ObjectFile *file,int shndx) {
  if (shndx < file->inputfile.elf_sections_num)
    return file->inputfile.elf_sections[shndx];
}

static inline char* get_name(ObjectFile *file,int shndx) {
    if (file->inputfile.elf_sections_num <= shndx)
        return ".common";
    return (char *)file->inputfile.shstrtab.data  + *file->inputfile.elf_sections[shndx]->sh_name.val;
}

static inline void ELFSym_set_frag(SectionFragment *frag,ELFSymbol *sym) {
    uintptr_t addr = (uintptr_t)frag;
    assert((addr & TAG_MASK) == 0);
    sym->origin = addr | TAG_FRAG;
}

inline ElfRel *get_rels(Context *ctx,ObjectFile *file,InputSection *sec) {
    if (sec->relsec_idx == -1)
        return NULL;
    // return get_data(ctx,file->elf_sections[sec->relsec_idx])
}
#endif  // 结束头文件保护

