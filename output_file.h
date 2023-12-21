#include "mold.h"
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
} Chunk;

typedef struct {
    /* data */
    Chunk *chunk;
} OutputEhdr;

typedef struct {
    /* data */
    Chunk *chunk;
    // Symbol *
    vector got_syms;
    vector tlsgd_syms;
    vector tlsdesc_syms;
    vector gottp_syms;
    u32 tlsld_idx;
} GotSection;

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

typedef struct  
{
    /* data */
    Chunk *chunk;
    // Symbol*
    vector symbols;
} PltSection;

typedef struct  
{
    /* data */
    Chunk *chunk;
    // Symbol *
    vector symbols;
} PltGotSection;

typedef struct  
{
    /* data */
    Chunk *chunk;
} SymtabSection;

typedef struct {
    Chunk *chunk;
    // Symbol *
    vector symbols;
    bool finalized;
} DynsymSection;

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
typedef struct OutputS OutputSection;
struct OutputS{
    Chunk *chunk;
    // InputSection *
    vector member;
    // Thunk
    vector thunks;
    RelocSection **reloc_sec;
};

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









