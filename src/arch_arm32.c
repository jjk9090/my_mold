#include "mold.h"
void isec_scan_relocations(Context *ctx,ObjectFile *file,int i,InputSection *isec) {
    assert(*get_shdr(file,isec->shndx)->sh_flags.val & SHF_ALLOC);
    isec->reldyn_offset = file->num_dynrel * sizeof(ElfRel);
    StringView rels = get_rels(ctx,file,isec);

    // Scan relocations
    for(i64 i = 0;i < rels.size;i++) {
        ElfRel *rel = (ElfRel *)(intptr_t)rels.data[i];
        if (rel->r_type == R_NONE) 
            continue;       
    }
}

u64 get_eflags(Context *ctx) {
  return EF_ARM_EABI_VER5;
}

void thunk_copy_buf(Context *ctx,Thunk *thunk) {
    // TLS trampoline code. ARM32's TLSDESC is designed so that this
    // common piece of code is factored out from object files to reduce
    // output size. Since no one provide, the linker has to synthesize it.
    uint32_t hdr[] = {
        0xe08e0000u, // add r0, lr, r0
        0xe5901004u, // ldr r1, [r0, #4]
        0xe12fff11u, // bx  r1
        0xe320f000u, // nop
    };
    const u8 entry[] = {
        // .thumb
        0x78, 0x47,             //    bx   pc  # jumps to 1f
        0xc0, 0x46,             //    nop
        // .arm
        0x00, 0xc0, 0x9f, 0xe5, // 1: ldr  ip, 3f
        0x0f, 0xf0, 0x8c, 0xe0, // 2: add  pc, ip, pc
        0x00, 0x00, 0x00, 0x00, // 3: .word sym - 2b
    };
    

    u8 *buf = ctx->buf + *(u32 *)&(thunk->output_section->chunk->shdr.sh_offset) + thunk->offset;
    memcpy(buf, hdr, sizeof(hdr));
    buf += sizeof(hdr);

    u64 P =*(u32 *)&(thunk->output_section->chunk->shdr.sh_addr) + thunk->offset + sizeof(hdr);

    for (int i = 0;i < thunk->symbols.size;i++) {
        ELFSymbol *sym = thunk->symbols.data[i];
        memcpy(buf, entry, sizeof(entry));
        *(uint32_t *)(buf + 12) = elfsym_get_addr(ctx,0,sym) - P - 16;

        buf += sizeof(entry);
        P += sizeof(entry);
    }
}