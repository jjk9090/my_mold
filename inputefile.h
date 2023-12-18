#ifndef INPUT_FILE_H
#define INPUT_FILE_H
#include <stdbool.h>
#include "mold.h"

typedef struct SmartPointer {
    // 成员变量
    ObjectFile *ptr;
    int ref_count;
} SmartPointer;



typedef struct  
{
    /* data */
    vector shard_offsets;
    Chunk *chunk;

    my_hash_element *map;
} MergedSection;

typedef struct SectionFragment {
    MergedSection *output_section;
    u32 offset;
    bool is_alive;
} SectionFragment;

typedef struct {
    MergedSection *parent;
    u8 p2align;
    // string_view
    vector strings;
    // u64
    vector hashes;
    // u32
    vector frag_offsets;
    // SectionFragment *
    vector fragments;
} MergeableSection;
InputSection** sections;
ElfShdr **elf_sections;
ElfSym **elf_syms;
MappedFile *inputfile_mf;
StringView shstrtab;
// ElfShdr *symtab_sec;
i64 first_global;
StringView symbol_strtab;
vector local_syms;
vector frag_syms;
vector symbols;
BitVector has_symver;
extern StringView get_string(Context *ctx, ElfShdr *shdr);
extern ObjectFile * objectfile_create(Context *ctx,MappedFile *mf,char *archive_name, bool is_in_lib);
extern void parse(Context *ctx,ObjectFile *obj);
extern StringView get_data(Context *ctx,ElfShdr *shdr);
void test();
#endif