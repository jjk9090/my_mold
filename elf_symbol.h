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