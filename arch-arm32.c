#include "mold.h"
void isec_scan_relocations(Context *ctx,ObjectFile *file,int i,InputSection *isec) {
    assert(*get_shdr(file,i)->sh_flags.val & SHF_ALLOC);
    isec->reldyn_offset = file->num_dynrel * sizeof(ElfRel);
    StringView rels = get_rels(ctx,file,isec);

    // Scan relocations
    for(i64 i = 0;i < rels.size;i++) {
        ElfRel *rel = (ElfRel *)rels.data[i];
        if (rel->r_type == R_NONE) 
            continue;       
    }
}