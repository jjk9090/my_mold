#ifndef ELF_SYMBOL_H
#define ELF_SYMBOL_H
#include "mold.h"
enum  {
    TAG_ABS  = 0b00,
    TAG_ISEC = 0b01,
    TAG_OSEC = 0b10,
    TAG_FRAG = 0b11,
    TAG_MASK = 0b11,
};
typedef struct  {
    /* data */
    ObjectFile *file;
    uintptr_t origin;
    u64 value;
    const char *nameptr;
    i32 namelen;

    // Index into the symbol table of the owner file.
    i32 sym_idx;
    i32 aux_idx;

    u16 ver_idx;
    bool is_weak : 1;
    bool write_to_symtab : 1;
    bool is_traced : 1;
    bool is_wrapped : 1;

    bool is_imported : 1;
    bool is_exported : 1;
    bool is_canonical : 1;

    bool has_copyrel : 1;
    bool is_copyrel_readonly : 1;
    bool referenced_by_regular_obj : 1;
    bool address_taken : 1;
} ELFSymbol;

static inline ELFSymbol *sym_init(ELFSymbol* sym) {
    sym->file = (ObjectFile *)malloc(sizeof(ObjectFile));
    sym->origin = 0;
    sym->value = 0;
    sym->nameptr = NULL;
    sym->namelen = 0;
    sym->sym_idx = -1;
    sym->aux_idx = -1;
    sym->ver_idx = VER_NDX_UNSPECIFIED;
    sym->is_weak = false;
    sym->write_to_symtab = false;
    sym->is_traced = false;
    sym->is_wrapped = false;
    sym->is_imported = false;
    sym->is_exported = false;
    sym->is_canonical = false;
    sym->has_copyrel = false;
    sym->is_copyrel_readonly = false;
    sym->referenced_by_regular_obj = false;
    sym->address_taken = false;
}

#endif