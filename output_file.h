#include "mold.h"
typedef struct PltGotSection PltGotSection;
typedef struct GotSection GotSection;
typedef struct PltSection PltSection;
typedef struct DynsymSection DynsymSection;
typedef struct Chunk {
    ElfShdr shdr;
    char *name;
    bool is_relro;
    i64 shndx;

    i64 local_symtab_idx;
    i64 num_local_symtab;
    i64 strtab_size;
    i64 strtab_offset;

    // Offset in .rel.dyn
    i64 reldyn_offset ;
    OutputSection *outsec;
    bool is_outsec;
    GotSection *gotsec;
    bool is_gotsec;
    PltGotSection *pltgotsec;
    bool is_pltgot;
    PltSection *pltsec;
    bool is_plt;
    DynsymSection *dynsym_sec;
    bool is_dynsym;

    MergedSection *merge_sec;
    bool is_merge_sec;
} Chunk;

typedef struct {
    /* data */
    Chunk *chunk;
} OutputEhdr;

struct GotSection{
    /* data */
    Chunk *chunk;
    // Symbol *
    vector got_syms;
    vector tlsgd_syms;
    vector tlsdesc_syms;
    vector gottp_syms;
    u32 tlsld_idx;
};

typedef struct {
    Chunk *chunk;
    // ElfPhdr
    vector phdrs;
} OutputPhdr;

typedef struct {
    Chunk *chunk;
} OutputShdr;

typedef struct {
    /* data */
    Chunk *chunk;
    i64 HDR_SIZE;
    i64 ENTRY_SIZE;
} GotPltSection;

typedef struct  
{
    /* data */
    Chunk *chunk;
} RelDynSection;

typedef struct  
{
    /* data */
    Chunk *chunk;
} RelPltSection;

typedef struct  
{
    /* data */
    Chunk *chunk;
    i64 ARM;
    i64 THUMB;
    i64 DATA;
} StrtabSection;

struct PltSection {
    /* data */
    Chunk *chunk;
    // Symbol*
    vector symbols;
};

struct PltGotSection {
    /* data */
    Chunk *chunk;
    // Symbol *
    vector symbols;
};

typedef struct  
{
    /* data */
    Chunk *chunk;
} SymtabSection;

struct DynsymSection{
    Chunk *chunk;
    // Symbol *
    vector symbols;
    bool finalized;
};

typedef struct  
{
    /* data */
    Chunk *chunk;
} EhFrameSection;

typedef struct  
{
    /* data */
    Chunk *chunk;
} ShstrtabSection;

typedef struct  
{
    /* data */
    Chunk *chunk;
    i64 HEADER_SIZE;
    u32 num_fdes;
} EhFrameHdrSection;
typedef struct RelocS RelocSection;
struct OutputS{
    Chunk *chunk;
    // InputSection *
    vector member;
    // Thunk
    vector thunks;
    RelocSection **reloc_sec;
};

typedef struct OutputS OutputSection;

struct RelocS{
    Chunk *chunk;
    OutputSection *output_section;
    // i64
    vector offsets;
};

typedef struct {
    /* data */
    Chunk *chunk;  
} HashSection;

typedef struct {
    /* data */
    Chunk *chunk;
    i64 LOAD_FACTOR;
    i64 HEADER_SIZE;
    i64 BLOOM_SHIFT;
    u32 num_buckets;
    u32 num_bloom;
} GnuHashSection;

typedef struct {
    /* data */
    Chunk *chunk;
    i64 dynsym_offset;
} DynstrSection;

typedef struct {
    /* data */
    Chunk *chunk;
    // U16
    vector contents;
} VersymSection;

typedef struct {
    Chunk *chunk;
    // u8
    vector contents;
} VerneedSection;

typedef struct {
    /* data */
    Chunk *chunk;
} RelroPaddingSection;

typedef struct {
    /* data */
    Chunk *chunk;
} DynamicSection;









