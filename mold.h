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
#include <stdarg.h>

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
#include "config.h"

#include "mapped_file.h"
// 定义上下文信息的结构体
ARM32 target;

enum {
    NO_PLT = 1 << 0, // Request an address other than .plt
    NO_OPD = 1 << 1, // Request an address other than .opd (PPC64V1 only)
};

typedef enum { HEADER, OUTPUT_SECTION, SYNTHETIC } ChunkKind;

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
    vector elf_syms;
    int elf_syms_num;
    MappedFile *inputfile_mf;
    StringView symbol_strtab;
    vector local_syms;
    vector frag_syms;

    // i32
    vector output_sym_indices;
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
    // CieRecord
    vector cies;
    vector fdes;
} ;

typedef struct {
    enum { NONE, SECTION, GROUP, ADDR, ALIGN, SYMBOL } type;
    char *name;
    u64 value;
} SectionOrder;

typedef struct {
    /* data */
    u8 *buf;
    // u8
    vector buf2;
    char *path;
    i64 fd;
    i64 filesize;
    bool is_mmapped;
    bool is_unmapped;
} OutputFile;

#include "symbol.h"
#include "output_file.h"

typedef enum {
    SEPARATE_LOADABLE_SEGMENTS,
    SEPARATE_CODE,
    NOSEPARATE_CODE,
} SeparateCodeKind;

struct Context {
    char** cmdline_args;
    int num_args;

    vector objs;
    vector string_pool;

    u8 *buf;
    OutputFile *output_file;
    char *output_tmpfile;
    struct {
        /* data */
        ELFSymbol *entry;
        ELFSymbol *fini;
        ELFSymbol *init;
        ELFSymbol **undefined;
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
        bool start_stop;
        bool strip_all;
        bool discard_all;
        bool discard_locals;
        bool omagic;
        bool rosegment;
        bool execute_only;
        bool nmagic;
        bool pic;
        bool quick_exit;
        vector retain_symbols_file;
        vector section_order;

        char *chroot;
        SeparateCodeKind z_separate_code;
        bool hash_style_sysv;
        bool hash_style_gnu;
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
        i64 z_stack_size;
        u64 image_base;
        i64 filler;
    } arg;

    i64 default_version;
    bool is_static;
    bool in_lib;
    i64 file_priority;
    // Symbol table
    ELFSymbol *symbol_map;
    int map_size;
    i64 page_size;
    u64 dtp_addr;

    // 其他成员变量的定义
    ObjectFile *internal_obj;
    ObjectFile **obj_pool;
    vector osec_pool;
    vector internal_esyms;
    vector merged_sections;
    int merged_sections_count;
    bool overwrite_output_file;
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
    DynamicSection *dynamic;
    StrtabSection *strtab;
    DynsymSection *dynsym;
    EhFrameSection *eh_frame;
    ShstrtabSection *shstrtab;
    EhFrameHdrSection *eh_frame_hdr;
    DynstrSection *dynstr;
    HashSection *hash;
    GnuHashSection *gnu_hash;
    // ShstrtabSection *shstrtab;
    PltSection *plt;
    PltGotSection *pltgot;
    SymtabSection *symtab;
    VersymSection *versym;
    VerneedSection *verneed;
    RelroPaddingSection *relro_padding;
    // Linker-synthesized symbols
    ELFSymbol *_DYNAMIC;
    ELFSymbol *_GLOBAL_OFFSET_TABLE_;
    ELFSymbol *_PROCEDURE_LINKAGE_TABLE_;
    ELFSymbol *_TLS_MODULE_BASE_;
    ELFSymbol *__GNU_EH_FRAME_HDR;
    ELFSymbol *__bss_start;
    ELFSymbol *__dso_handle;
    ELFSymbol *__ehdr_start;
    ELFSymbol *__executable_start;
    ELFSymbol *__exidx_end;
    ELFSymbol *__exidx_start;
    ELFSymbol *__fini_array_end;
    ELFSymbol *__fini_array_start;
    ELFSymbol *__global_pointer;
    ELFSymbol *__init_array_end;
    ELFSymbol *__init_array_start;
    ELFSymbol *__preinit_array_end;
    ELFSymbol *__preinit_array_start;
    ELFSymbol *__rel_iplt_end;
    ELFSymbol *__rel_iplt_start;
    ELFSymbol *_edata;
    ELFSymbol *_end;
    ELFSymbol *_etext;
    ELFSymbol *edata;
    ELFSymbol *end;
    ELFSymbol *etext;
} ;

typedef struct {
    /* data */
    i64 alignment;

    OutputSection *output_section;
    i64 offset;
    // Symbol *
    vector symbols;
} Thunk;


#include "filetype.h"
#include "input_file.h"
#include "cmdline.h"
#include "hashmap.h"
#include "elf_symbol.h"
#include "output-file-unix.h"

static inline i64 get_shndx(ElfSym *esym) {
    if (*esym->st_shndx.val == SHN_XINDEX)
        return -1;
    return *esym->st_shndx.val;
}
static const StringView EMPTY_STRING_VIEW = {NULL, 0};
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
void obj_resolve_symbols(Context *ctx,ObjectFile *obj);
void add_synthetic_symbols (Context *ctx);
void isec_scan_relocations(Context *ctx,ObjectFile *file,int i,InputSection *isec);
void scan_relocations(Context *ctx);
void compute_section_sizes(Context *ctx);
void create_range_extension_thunks(Context *ctx,OutputSection *osec);
void sort_output_sections(Context *ctx);
void verneed_construct(Context *ctx);
void create_output_symtab(Context *ctx);
void eh_frame_construct(Context *ctx);
void output_sec_compute_symtab_size(Context *ctx,Chunk *chunk);
void plt_sec_compute_symtab_size(Context *ctx,Chunk *chunk);
void pltgot_sec_compute_symtab_size(Context *ctx,Chunk *chunk);
void got_sec_compute_symtab_size(Context *ctx,Chunk *chunk);
void obj_compute_symtab_size(Context *ctx,ObjectFile *file);

void create_output_symtab(Context *ctx);
void compute_section_headers(Context *ctx);
void output_phdr_update_shdr(Context *ctx,Chunk *chunk);
i64 to_phdr_flags(Context *ctx, Chunk *chunk);

void output_phdr_update_shdr(Context *ctx,Chunk *chunk);
void reldyn_update_shdr(Context *ctx,Chunk *chunk);
void strtab_update_shdr(Context *ctx,Chunk *chunk);
void shstrtab_update_shdr(Context *ctx,Chunk *chunk);
void symtab_update_shdr(Context *ctx,Chunk *chunk);
void gotplt_update_shdr(Context *ctx,Chunk *chunk);
void plt_update_shdr(Context *ctx,Chunk *chunk);
void relplt_update_shdr(Context *ctx,Chunk *chunk);
void dynsym_update_shdr(Context *ctx,Chunk *chunk);
void hash_update_shdr(Context *ctx,Chunk *chunk);
void gnu_hash_update_shdr(Context *ctx,Chunk *chunk);
void eh_frame_hdr_update_shdr(Context *ctx,Chunk *chunk);
void gnu_version_update_shdr(Context *ctx,Chunk *chunk);
void gnu_version_r_update_shdr(Context *ctx,Chunk *chunk);

void fix_synthetic_symbols(Context *ctx);
i64 set_osec_offsets(Context *ctx);
void copy_chunks(Context *ctx);
void ehdr_copy_buf(Context *ctx,Chunk *chunk);
void shdr_copy_buf(Context *ctx,Chunk *chunk);
u64 get_eflags(Context *ctx);
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

static inline StringView get_rels(Context *ctx,ObjectFile *file,InputSection *sec) {
    if (sec->relsec_idx == -1)
        return EMPTY_STRING_VIEW;
    return get_data(ctx,file->inputfile.elf_sections[sec->relsec_idx]);
}

static inline InputSection *get_section(ElfSym *esym,ObjectFile *obj) {
    return obj->sections[get_shndx(esym)];
}

static inline ElfSym *esym(Inputefile *file,int sym_idx) {
  return file->elf_syms.data[sym_idx];
}

static inline void set_input_section(InputSection *isec,ELFSymbol *sym) {
    uintptr_t addr = (uintptr_t)isec;
    assert((addr & TAG_MASK) == 0);
    sym->origin = addr | TAG_ISEC;
}

static inline void set_output_section(Chunk *osec,ELFSymbol *sym) {
    uintptr_t addr = (uintptr_t)osec;
    assert((addr & TAG_MASK) == 0);
    sym->origin = addr | TAG_OSEC;
}

static inline Chunk *get_output_section(ELFSymbol *sym){
    if ((sym->origin & TAG_MASK) == TAG_OSEC)
        return (Chunk *)(sym->origin & ~TAG_MASK);
    return NULL;
}

static inline int is_alpha(char c) {
    return c == '_' || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static inline int is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
}

static inline bool is_c_identifier(const char* s) {
    if (strlen(s) == 0)
        return false;   

    if (!is_alpha(s[0]))
        return false;
    for (size_t i = 1; i < strlen(s); i++)
        if (!is_alnum(s[i]))
            return false;
    return true;
}

static inline InputSection *elfsym_get_input_section(ELFSymbol *sym){
    if ((sym->origin & TAG_MASK) == TAG_ISEC)
        return (InputSection *)(sym->origin & ~TAG_MASK);
    return NULL;
}

static inline u32 elfsym_get_type(ObjectFile *file,int i){
    ElfSym *sym= esym(&(file->inputfile),i);
    if (sym && sym->st_type == STT_GNU_IFUNC && file->inputfile.is_dso)
        return STT_FUNC;
    if(sym)
        return esym(&(file->inputfile),i)->st_type;
}

static inline SectionFragment *get_frag(ELFSymbol *sym) {
  if ((sym->origin & TAG_MASK) == TAG_FRAG)
    return (SectionFragment *)(sym->origin & ~TAG_MASK);
  return NULL;
}

static inline bool is_local(Context *ctx,ELFSymbol *sym,ObjectFile *file,int i) {
    if (ctx->arg.relocatable)
        return esym(&(file->inputfile),i)->st_bind == STB_LOCAL;
    return !sym->is_imported && !sym->is_exported;
}

static inline OutputSection *output_find_section(Context *ctx,u32 sh_type) {
    for (i64 i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        OutputSection *osec = chunk->outsec;
        if (osec)
            if (*osec->chunk->shdr.sh_type.val == sh_type)
                return osec;
    }      
    return NULL;
}

static inline OutputSection *output_find_section_with_name(Context *ctx,char *name) {
    for (i64 i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        OutputSection *osec = chunk->outsec;
        if (osec)
            if (!strcmp(osec->chunk->name,name))
                return osec;
    }      
    return NULL;
}

static inline u64 to_plt_offset(i32 pltidx) {
    return target.plt_hdr_size + pltidx * target.plt_size;
}

static inline ChunkKind kind(Chunk *chunk) {
    if(!strcmp(chunk->name,"EHDR") || !strcmp(chunk->name,"PHDR") || !strcmp(chunk->name,"SHDR"))
        return HEADER;
    if (chunk->is_outsec)
        return OUTPUT_SECTION;
    return SYNTHETIC;
}

static inline u64 elfsym_get_addr(Context *ctx,i64 flags,ELFSymbol *sym) {
    SectionFragment *frag = get_frag(sym);
    if (sym) {
        if (!frag->is_alive) {
            return 0;
        }

        // return frag_get_addr(ctx) + sym->value;
    }
    InputSection *isec = elfsym_get_input_section(sym);
    if(!isec)
        return sym->value;

    if(!isec->is_alive) {

    }

    // return input_sec_get_addr() + sym->value;
}
#endif  // 结束头文件保护

