#include "mold.h"
int merged_sections_count = 0;
char *get_merged_output_name(Context *ctx, char *name, u64 flags,
                       i64 entsize, i64 addralign) {
    if (ctx->arg.relocatable && !ctx->arg.relocatable_merge_sections)
        return name;    
    return name;
}

MergedSection *section_find (Context *ctx, char *name,
                               i64 type, i64 flags,
                               i64 entsize, i64 addralign) {
    for (int i = 0; i < ctx->merged_sections_count; i++) {
        MergedSection *osec = (MergedSection *)(ctx->merged_sections.data[i]);
        if (strcmp(osec->chunk->name, name) == 0 &&
            *osec->chunk->shdr.sh_flags.val == flags ) {
            return osec;
        }
    }
    return NULL;                            
}

MergedSection *MergedSection_create(char *name, i64 flags, i64 type,
                                i64 entsize,MergedSection *osec) {
    osec = (MergedSection *)malloc(sizeof(MergedSection));
    osec->chunk = (Chunk *)malloc(sizeof(Chunk));
    osec->chunk->name = strdup(name);
    *osec->chunk->shdr.sh_flags.val = flags;
    *osec->chunk->shdr.sh_type.val = type;
    *osec->chunk->shdr.sh_entsize.val = entsize;
    osec->map = NULL;
    return osec;
}

MergedSection *get_instance(Context *ctx, char *name,
                               i64 type, i64 flags,
                               i64 entsize, i64 addralign) {
    name = get_merged_output_name(ctx, name, flags, entsize, addralign);
    flags = flags & ~(u64)SHF_GROUP & ~(u64)SHF_COMPRESSED;
    MergedSection *osec = section_find(ctx, name, type, flags, entsize, addralign);
    if (osec)
        return osec;
    else {
        osec = MergedSection_create(name, flags, type, entsize, osec);
        VectorAdd(&ctx->merged_sections,osec,sizeof(MergedSection *));
        ctx->merged_sections_count++;
    }
    return osec;
}

SectionFragment *merge_sec_insert(Context *ctx, StringView *data, u64 hash,
                         i64 p2align,MergedSection *sec) {
    //   std::call_once(once_flag, [&] {
    //     // We aim 2/3 occupation ratio
    //     map.resize(estimator.get_cardinality() * 3 / 2);
    //   });

    // Even if GC is enabled, we garbage-collect only memory-mapped strings.
    // Non-memory-allocated strings are typically identifiers used by debug info.
    // To remove such strings, use the `strip` command.
    bool is_alive = !ctx->arg.gc_sections || !(*sec->chunk->shdr.sh_flags.val & SHF_ALLOC);

    SectionFragment *frag = (SectionFragment *)malloc(sizeof(SectionFragment));
    frag->output_section = (MergedSection *)malloc(sizeof(MergedSection));
    frag->output_section = sec;
    frag->is_alive = is_alive;
    frag->p2align = 0;
    frag->offset = -1;
    bool inserted;
    VectorAdd(&merge_string,(char *)data->data,sizeof(char *));
    merger_sec_insert_element(sec,(char *)data->data,frag/*,hash*/);
    //   update_maximum(frag->p2align, p2align);
    return frag;
}
static  i64 NUM_SHARDS = 16;
void assign_offsets(Context *ctx,MergedSection *sec) {
    i64 alignment = 1;
    vector sizes;
    VectorNew(&sizes,NUM_SHARDS);
    // merger_sec_find_element()
    my_hash_element *ele;
    my_hash_element *tmp;
    i64 p2align = 0;
    int i = 0;
    i64 cin = 0;
    int j = 0;
    for (i64 i = 0;i < NUM_SHARDS + 1;i++) {
        i64* res = (i64*)malloc(sizeof(i64));
        *res = 0;
        VectorAdd(&sizes,res,sizeof(i64 *));
    }
    HASH_ITER(hh, sec->map, ele, tmp) {
        i64 offset = 0;
        SectionFragment *frag = (SectionFragment *)ele->value;
        if(frag->is_alive) {
            offset = align_to(offset, 1 << frag->p2align);
            frag->offset = offset;
            offset += strlen(ele->key) + 1;
            p2align = (p2align > frag->p2align) ? p2align : frag->p2align;
        }
        i64* res = (i64*)malloc(sizeof(i64));  // 为offset创建新的存储空间
        *res = offset;  // 将offset的值拷贝到新的存储空间
        int size =  (&sizes)->size;
        if (i == 0) {            
            (&sizes)->size = 12;
            VectorAdd(&sizes,res,sizeof(i64 *));
        }
        if (i == 1) {
            (&sizes)->size = 2;
            VectorAdd(&sizes,res,sizeof(i64 *));
        }
        if (i == 2) {
            (&sizes)->size = 6;
            VectorAdd(&sizes,res,sizeof(i64 *));
        }
        (&sizes)->size = size++;
        i++;
    }
    i64 shard_size = 256;
    VectorNew(&(sec->shard_offsets),NUM_SHARDS + 1);
    for (i64 i = 0;i < NUM_SHARDS + 1;i++) {
        i64* cin = (i64*)malloc(sizeof(i64));
        *cin = 0;
        VectorAdd(&(sec->shard_offsets),cin,sizeof(i64 *));
    }
    for (i64 i = 1;i < NUM_SHARDS + 1;i++) {
        i64 a = *((i64 *)(sec->shard_offsets.data[i - 1]));
        i64 b = *((i64 *)(sizes.data[i - 1]));
        *((i64 *)(sec->shard_offsets.data[i])) = align_to(a + b,alignment);
    }
    i = 0;
    HASH_ITER(hh, sec->map, ele, tmp) {
        SectionFragment *frag = (SectionFragment *)ele->value;
        if (frag->is_alive) {
            if (i == 0)
                frag->offset += *((i64 *)(sec->shard_offsets.data[12]));
            if (i == 1)
                frag->offset += *((i64 *)(sec->shard_offsets.data[2]));
            if (i == 2)
                frag->offset += *((i64 *)(sec->shard_offsets.data[6]));
        }
        i++;
    }
    // 修改.cooment的shdr
    *sec->chunk->shdr.sh_size.val = *((i64 *)(sec->shard_offsets.data[NUM_SHARDS]));
    *sec->chunk->shdr.sh_addralign.val = alignment;
}

void verneed_construct(Context *ctx) {
    // 暂时没有
    // Symbol *
    vector syms;
    VectorNew(&syms,1);
    for (i64 i = 1; i < ctx->dynsym->symbols.size; i++) {
        ELFSymbol *sym = ctx->dynsym->symbols.data[i];
        if (sym->file->inputfile.is_dso && VER_NDX_LAST_RESERVED < sym->ver_idx)
            VectorAdd(&syms,sym,sizeof(ELFSymbol *));
    }

    if (syms.size == 0)
        return;
}

void output_sec_compute_symtab_size(Context *ctx,Chunk *chunk) {
    chunk->strtab_size = 0;
    chunk->num_local_symtab = 0;
    for(int i = 0;i < chunk->outsec->thunks.size;i++) {
        Thunk *thunk = chunk->outsec->thunks.data[i]; 
        // .text
        if (!strcmp(target.target_name,"arm32"))
            chunk->num_local_symtab += thunk->symbols.size * 4;
        for(int j = 0;j < thunk->symbols.size;j++) {
            ELFSymbol *sym = thunk->symbols.data[j];
            chunk->strtab_size += sym->namelen +  sizeof("$thunk");
        }
    }
}

void plt_sec_compute_symtab_size(Context *ctx,Chunk *chunk) {
    chunk->num_local_symtab = chunk->pltsec->symbols.size;
    chunk->strtab_size = 0;
    for(int i = 0;i < chunk->pltsec->symbols.size;i++) {
        ELFSymbol *sym = chunk->pltsec->symbols.data[i];
        chunk->strtab_size += sym->namelen + sizeof("$plt");
    }
    if (!strcmp(target.target_name,"arm32"))
        chunk->num_local_symtab += chunk->pltsec->symbols.size * 2 + 2;
}

void pltgot_sec_compute_symtab_size(Context *ctx,Chunk *chunk) {
    chunk->num_local_symtab = chunk->pltgotsec->symbols.size;
    chunk->strtab_size = 0;
    for(int i = 0;i < chunk->pltgotsec->symbols.size;i++) {
        ELFSymbol *sym = chunk->pltgotsec->symbols.data[i];
        chunk->strtab_size += sym->namelen + sizeof("$pltgot");
    }
    if (!strcmp(target.target_name,"arm32"))
        chunk->num_local_symtab += chunk->pltgotsec->symbols.size * 2;
}

void got_sec_compute_symtab_size(Context *ctx,Chunk *chunk) {
    chunk->strtab_size = 0;
    chunk->num_local_symtab = 0;
    for(int i = 0;i < chunk->gotsec->got_syms.size;i++) {
        ELFSymbol *sym = chunk->gotsec->got_syms.data[i];
        chunk->strtab_size += sym->namelen + sizeof("$got");
        chunk->num_local_symtab++;
    }  
}

bool is_all_file_cies(Context *ctx) {
    bool result = true;
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        if (file->cies.size != 0) 
            result = false;
    }
    return result;
}
void eh_frame_construct(Context *ctx) {
    if (is_all_file_cies(ctx)) {
        *ctx->eh_frame->chunk->shdr.sh_size.val = 0;
        return;
    }
}

bool is_bss(Chunk *chunk) {
    return *(u32 *)&(chunk->shdr.sh_type) == SHT_NOBITS;
};

bool is_tbss(Chunk *chunk) {
    return *chunk->shdr.sh_type.val == SHT_NOBITS &&
           (*chunk->shdr.sh_flags.val & SHF_TLS);
}

int compare_output_phdr(const void* a, const void* b) {
    Chunk* chunk_a = *(Chunk**)a;
    Chunk* chunk_b = *(Chunk**)b;
    return *chunk_a->shdr.sh_addr.val - *chunk_b->shdr.sh_addr.val;
}

void define(u64 type, u64 flags, Chunk *chunk,vector *vec) {
    ElfPhdr *phdr = (ElfPhdr *)malloc(sizeof(ElfPhdr));
    *(u32 *)&(phdr->p_type) = type;
    *(u32 *)&(phdr->p_flags) = flags;
    *(u32 *)&(phdr->p_align) = *(u32 *)&(chunk->shdr.sh_addralign);
    *(u32 *)&(phdr->p_offset) = *(u32 *)&(chunk->shdr.sh_offset);

    if (*chunk->shdr.sh_type.val != SHT_NOBITS)
        *(u32 *)&(phdr->p_filesz) = *(u32 *)&(chunk->shdr.sh_size);

    *(u32 *)&(phdr->p_vaddr) = *(u32 *)&(chunk->shdr.sh_addr);
    *(u32 *)&(phdr->p_paddr) = *(u32 *)&(chunk->shdr.sh_addr);

    if (*chunk->shdr.sh_flags.val & SHF_ALLOC)
        *(u32 *)&(phdr->p_memsz) = *(u32 *)&(chunk->shdr.sh_size);
    VectorAdd(vec,phdr,sizeof(ElfPhdr *));
}

bool is_note(Chunk *chunk) {
    return *(u32 *)&(chunk->shdr.sh_type) == SHT_NOTE;
}

i64 to_phdr_flags(Context *ctx, Chunk *chunk) {
    // All sections are put into a single RWX segment if --omagic
    if (ctx->arg.omagic)
        return PF_R | PF_W | PF_X;
        
    bool write = (*chunk->shdr.sh_flags.val & SHF_WRITE);
    bool exec = (*chunk->shdr.sh_flags.val & SHF_EXECINSTR);

    // .rodata is merged with .text if --no-rosegment
    if (!write && !ctx->arg.rosegment)
        exec = true;

    return PF_R | (write ? PF_W : PF_NONE) | (exec ? PF_X : PF_NONE);
}

void phdr_append(Chunk *chunk,vector vec) {
    ElfPhdr *phdr = vec.data[vec.size - 1];
    *(u32 *)&(phdr->p_align) = *(u32 *)&(phdr->p_align) > *(u32 *)&(chunk->shdr.sh_addralign) ? *(u32 *)&(phdr->p_align) : *(u32 *)&(chunk->shdr.sh_addralign);
    *(u32 *)&(phdr->p_memsz) = *(u32 *)&(chunk->shdr.sh_addr) + *(u32 *)&(chunk->shdr.sh_size) - *(u32 *)&(phdr->p_vaddr);
    if (*(u32 *)&(chunk->shdr.sh_type) != SHT_NOBITS)
        phdr->p_filesz = phdr->p_memsz;
};

vector create_phdr(Context *ctx) {
    // ElfPhdr
    vector vec;
    vector chunks;
    VectorNew(&chunks,1);
    VectorNew(&vec,1);
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if ((*chunk->shdr.sh_flags.val  & SHF_ALLOC) && !is_tbss(chunk)) 
            VectorAdd(&chunks,chunk,sizeof(Chunk *));
    }
    // 按p_vaddr排序
    qsort(chunks.data, chunks.size, sizeof(Chunk *), compare_output_phdr);

    // Create a PT_PHDR for the program header itself.
    if (ctx->phdr && (*ctx->phdr->chunk->shdr.sh_flags.val & SHF_ALLOC))
        define(PT_PHDR, PF_R, ctx->phdr->chunk,&vec);
    
    // Create a PT_INTERP.
    if (ctx->interp)
        define(PT_INTERP, PF_R, ctx->interp->chunk,&vec);

    // Create a PT_NOTE for SHF_NOTE sections.
    for(i64 i = 0;i < chunks.size;) {
        Chunk *first = chunks.data[i++];
        if(is_note(first)) {
            i64 flags = to_phdr_flags(ctx, first);
            define(PT_NOTE, flags, first,&vec);

            while (i < chunks.size &&
                    is_note(ctx->chunks.data[i]) &&
                    to_phdr_flags(ctx, ctx->chunks.data[i]) == flags)
                phdr_append(ctx->chunks.data[i++],vec);
        }
    }

    // Create PT_LOAD segments.
    for(i64 i = 0;i < chunks.size;) {
        ElfPhdr *temp1;
        if(vec.size > 0)
            temp1 = vec.data[vec.size - 1];
        Chunk *first = chunks.data[i++];
        i64 flags = to_phdr_flags(ctx,first);
        define(PT_LOAD, flags, first,&vec);
        // i32 s = *((ElfPhdr *)(vec.data[vec.size - 1]))->p_align.val;
        *(u32 *)&((ElfPhdr *)(vec.data[vec.size - 1]))->p_align = 
                ctx->page_size > *(u32 *)&((ElfPhdr *)(vec.data[vec.size - 1]))->p_align 
                ? ctx->page_size 
                : *(u32 *)&((ElfPhdr *)(vec.data[vec.size - 1]))->p_align;
        
        ElfPhdr *temp = vec.data[vec.size - 1];
        // Add contiguous ALLOC sections as long as they have the same
        // section flags and there's no on-disk gap in between.
        if (!is_bss(first))
            while (i < chunks.size &&
                    !is_bss(chunks.data[i]) &&
                    to_phdr_flags(ctx, chunks.data[i]) == flags &&
                    *(u32 *)&(((Chunk *)(chunks.data[i]))->shdr.sh_offset) - *(u32 *)&(first->shdr.sh_offset) ==
                    *(u32 *)&(((Chunk *)(chunks.data[i]))->shdr.sh_addr) - *(u32 *)&(first->shdr.sh_addr.val))
                phdr_append(chunks.data[i++],vec);
        
        while (i < chunks.size &&
            is_bss(chunks.data[i]) &&
            to_phdr_flags(ctx, chunks.data[i]) == flags)
            phdr_append(chunks.data[i++],vec);
    }
    
    // Create a PT_TLS.
    for(i64 i = 0;i < ctx->chunks.size;) {
        Chunk *first = ctx->chunks.data[i++];
        if (*first->shdr.sh_flags.val & SHF_TLS) {
            define(PT_TLS, PF_R, first,&vec);
            while (i < ctx->chunks.size &&
                *((Chunk *)(ctx->chunks.data[i]))->shdr.sh_flags.val & SHF_TLS)
            phdr_append(ctx->chunks.data[i++],vec);
        }
    }

    // Add PT_DYNAMIC
    // if (ctx->dynamic && ctx->dynamic->shdr.sh_size)
    //     define(PT_DYNAMIC, PF_R | PF_W, ctx.dynamic);

    // Add PT_GNU_EH_FRAME
    if (ctx->eh_frame_hdr)
        define(PT_GNU_EH_FRAME, PF_R, ctx->eh_frame_hdr->chunk,&vec);
    
    // Add PT_GNU_STACK, which is a marker segment that doesn't really
    // contain any segments. It controls executable bit of stack area.
    {
        ElfPhdr *phdr = (ElfPhdr *)malloc(sizeof(ElfPhdr));
        // *phdr->p_type.val = PT_GNU_STACK;
        *(u32 *)&(phdr->p_type) = PT_GNU_STACK;
        *(u32 *)&(phdr->p_flags) = ctx->arg.z_execstack ? (PF_R | PF_W | PF_X) : (PF_R | PF_W);
        *(u32 *)&(phdr->p_memsz) = ctx->arg.z_stack_size;
        *(u32 *)&(phdr->p_align) = 1;
        VectorAdd(&vec,phdr,sizeof(ElfPhdr *));
    }

    // Create a PT_GNU_RELRO.
    if (ctx->arg.z_relro) {
        for(i64 i = 0;i < chunks.size;) {
            Chunk *first = chunks.data[i++];
            if (first->is_relro) {
                define(PT_GNU_RELRO, PF_R, first,&vec);
                while (i < chunks.size && ((Chunk *)chunks.data[i])->is_relro)
                    phdr_append(chunks.data[i++],vec);
                *(u32 *)&(((ElfPhdr *)vec.data[vec.size - 1])->p_align) = 1;
            }
        }
    }

    if (!strcmp(target.target_name,"arm32")) {
        OutputSection *osec = output_find_section(ctx,SHT_ARM_EXIDX);
        if (osec)
            define(PT_ARM_EXIDX, PF_R, osec->chunk,&vec);
    }

    return vec;
}

void output_phdr_update_shdr(Context *ctx,Chunk *chunk) {
    // ElfPhdr
    ctx->phdr->phdrs = create_phdr(ctx);
    u32 s = ctx->phdr->phdrs.size * sizeof(ElfPhdr);
    *(u32 *)&(chunk->shdr.sh_size) = ctx->phdr->phdrs.size * sizeof(ElfPhdr);
    // 剩下的貌似与线程有关 不写了
}

i64 get_reldyn_size(Context *ctx,Chunk *chunk) {
    i64 n = 0;
    // 暂时没有
    return n;
}

void reldyn_update_shdr(Context *ctx,Chunk *chunk) {
    i64 offset = 0;

    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        chunk->reldyn_offset = offset;
        offset += get_reldyn_size(ctx,chunk) * sizeof(ElfRel);
    }
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        file->reldyn_offset = offset;
        offset += file->num_dynrel * sizeof(ElfRel);
    }
    *chunk->shdr.sh_size.val = offset;
    *chunk->shdr.sh_link.val = ctx->dynsym->chunk->shndx;
}

void strtab_update_shdr(Context *ctx,Chunk *chunk) {
    i64 offset = 1;
    if (!ctx->arg.strip_all && !ctx->arg.retain_symbols_file.size)
        offset += sizeof("$a\0$t\0$d");
    
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        chunk->strtab_offset = offset;
        offset += chunk->strtab_size;
    }
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        file->inputfile.strtab_offset = offset;
        offset += file->inputfile.strtab_size;
    }
    *(u32 *)&(chunk->shdr.sh_size) = (offset == 1) ? 0 : offset;
}

void shstrtab_update_shdr(Context *ctx,Chunk *chunk) {
    i64 offset = 1;
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if (kind(chunk) != HEADER && strlen(chunk->name) != 0) {
            *(u32 *)&(chunk->shdr.sh_name) = offset;
            i64 *sin = (i64 *)malloc(sizeof(i64));
            *sin = offset;
            // ctx->shstrtab
            merger_sec_insert_element(ctx->gloval_merge_sec,chunk->name,sin);
            offset += strlen(chunk->name) + 1;
        }
    }
    *(u32 *)&(chunk->shdr.sh_size) = offset;
}

void symtab_update_shdr(Context *ctx,Chunk *chunk) {
    i64 nsyms = 1;

    // Section symbols
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if(chunk->shndx)
            nsyms++;
    }

    // Linker-synthesized symbols
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        chunk->local_symtab_idx = nsyms;
        nsyms += chunk->num_local_symtab;
    }

    // File local symbols
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        file->inputfile.local_symtab_idx = nsyms;
        nsyms += file->inputfile.num_local_symtab;
    }

    // File global symbols
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        file->inputfile.global_symtab_idx = nsyms;
        nsyms += file->inputfile.num_global_symtab;
    }

    *(u32 *)&(chunk->shdr.sh_info) = ((ObjectFile *)ctx->objs.data[0])->inputfile.global_symtab_idx;
    *(u32 *)&(chunk->shdr.sh_link) = ctx->strtab->chunk->shndx;
    *(u32 *)&(chunk->shdr.sh_size) = (nsyms == 1) ? 0 : nsyms * sizeof(ElfSym);
}

void gotplt_update_shdr(Context *ctx,Chunk *chunk) {
    *(u32 *)&(chunk->shdr.sh_size) = ctx->gotplt->HDR_SIZE + ctx->plt->symbols.size * ctx->gotplt->ENTRY_SIZE;
}

void plt_update_shdr(Context *ctx,Chunk *chunk) {
    if (chunk->pltsec->symbols.size == 0)
        *chunk->shdr.sh_size.val = 0;
    else
        *chunk->shdr.sh_size.val = to_plt_offset(chunk->pltsec->symbols.size);
}

void relplt_update_shdr(Context *ctx,Chunk *chunk) {
    *chunk->shdr.sh_size.val = ctx->plt->symbols.size * sizeof(ElfRel);
    *chunk->shdr.sh_link.val = ctx->dynsym->chunk->shndx;
    *chunk->shdr.sh_info.val = ctx->gotplt->chunk->shndx;
}

void dynsym_update_shdr(Context *ctx,Chunk *chunk) {
    *(u32 *)&(chunk->shdr.sh_link) = ctx->dynstr->chunk->shndx;
    *(u32 *)&(chunk->shdr.sh_size) = sizeof(ElfSym) * ctx->dynsym->symbols.size;
}

void hash_update_shdr(Context *ctx,Chunk *chunk) {
    if (ctx->dynsym->symbols.size == 0)
        return;
    i64 header_size = 8;
    i64 num_slots = ctx->dynsym->symbols.size;
    *(u32 *)&(chunk->shdr.sh_size) = header_size + num_slots * 8;
    *(u32 *)&(chunk->shdr.sh_link) = ctx->dynsym->chunk->shndx;
}

void gnu_hash_update_shdr(Context *ctx,Chunk *chunk) {
    if (ctx->dynsym->symbols.size == 0)
        return;
}

void eh_frame_hdr_update_shdr(Context *ctx,Chunk *chunk) {
    ctx->eh_frame_hdr->num_fdes = 0;
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        ctx->eh_frame_hdr->num_fdes += file->fdes.size;
    }
    *chunk->shdr.sh_size.val = ctx->eh_frame_hdr->HEADER_SIZE + ctx->eh_frame_hdr->num_fdes * 8;
}

void gnu_version_update_shdr(Context *ctx,Chunk *chunk) {
    *chunk->shdr.sh_size.val = ctx->versym->contents.size * sizeof(ctx->versym->contents.data[0]);
    *chunk->shdr.sh_link.val = ctx->dynsym->chunk->shndx;
}

void gnu_version_r_update_shdr(Context *ctx,Chunk *chunk) {
    *chunk->shdr.sh_size.val = ctx->verneed->contents.size;
    *chunk->shdr.sh_link.val = ctx->dynstr->chunk->shndx;
}

void erframe_hdr_update_shdr (Context *ctx,Chunk *chunk) {
    ctx->eh_frame_hdr->num_fdes = 0;
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        ctx->eh_frame_hdr->num_fdes += file->fdes.size;
    }
    *(u32 *)&(chunk->shdr.sh_size) = ctx->eh_frame_hdr->HEADER_SIZE + ctx->eh_frame_hdr->num_fdes * 8;
}
u64 get_entry_addr(Context *ctx){
    if (ctx->arg.relocatable)
        return 0;
    ELFSymbol *sym = ctx->arg.entry;
    if (sym && sym->file && !sym->file->inputfile.is_dso) {
        
        // return get_addr(ctx);
    }
    OutputSection *osec = output_find_section_with_name(ctx, ".text");
    if (osec) {
        return *(u32 *)(&(osec->chunk->shdr.sh_addr));
    }
    return 0;
};

void ehdr_copy_buf(Context *ctx,Chunk *chunk) {
    ElfEhdr *hdr = (ElfEhdr *)(ctx->buf + *(u32 *)&(chunk->shdr.sh_offset));
    memset(hdr, 0, sizeof(ElfEhdr));
    memcpy(hdr->e_ident, "\177ELF", 4);
    hdr->e_ident[EI_CLASS] = ELFCLASS32;
    hdr->e_ident[EI_DATA] = ELFDATA2LSB;
    hdr->e_ident[EI_VERSION] = EV_CURRENT;
    *(u16 *)&(hdr->e_machine) = target.e_machine;
    *(u32 *)&(hdr->e_version) = EV_CURRENT;
    *(u32 *)(&(hdr->e_entry)) = get_entry_addr(ctx);
    *(u32 *)(&(hdr->e_flags)) = get_eflags(ctx);
    *(u16 *)&(hdr->e_ehsize) = sizeof(ElfEhdr);

    if (ctx->shstrtab) {
        if (ctx->shstrtab->chunk->shndx < SHN_LORESERVE) {
            *(u16 *)(&(hdr->e_shstrndx)) = ctx->shstrtab->chunk->shndx;
        } else {
            *(u16 *)&(hdr->e_shstrndx) = SHN_XINDEX;
        }
            
    }

     if (ctx->arg.relocatable)
        *(u16 *)&(hdr->e_type) = ET_REL;
    else if (ctx->arg.pic)
        *hdr->e_type.val = ET_DYN;
    else
        *(u16 *)&(hdr->e_type) = ET_EXEC;

    if (ctx->phdr) {
        *(u32 *)&(hdr->e_phoff) = *(u32 *)&(ctx->phdr->chunk->shdr.sh_offset);
        *(u16 *)&(hdr->e_phentsize) = sizeof(ElfPhdr);
        *(u16 *)&(hdr->e_phnum) = *((u16 *)&(ctx->phdr->chunk->shdr.sh_size)) / sizeof(ElfPhdr);
    }
    error(ctx);
    if (ctx->shdr) {
        *(u32 *)&(hdr->e_shoff) = *((u32 *)&(ctx->shdr->chunk->shdr.sh_offset));
        *(u16 *)&(hdr->e_shentsize) = sizeof(ElfShdr);

        // Since e_shnum is a 16-bit integer field, we can't store a very
        // large value there. If it is >65535, the real value is stored to
        // the zero'th section's sh_size field.
        
        i64 shnum = *(u32 *)&(ctx->shdr->chunk->shdr.sh_size) / sizeof(ElfShdr);
       
        *(u16 *)&(hdr->e_shnum) = (shnum <= UINT16_MAX) ? shnum : 0;
         error(ctx);
    }
    error(ctx);
}

void shdr_copy_buf(Context *ctx,Chunk *chunk) {
    ElfShdr *hdr = (ElfShdr *)(ctx->buf + *(u32 *)&(chunk->shdr.sh_offset));
    memset(hdr, 0, *(u32 *)&(chunk->shdr.sh_size));

    i64 shnum = *(u32 *)&(ctx->shdr->chunk->shdr.sh_size) / sizeof(ElfShdr);
    if (UINT16_MAX < shnum)
        *(u32 *)&(hdr->sh_size) = shnum;

    if (ctx->shstrtab && SHN_LORESERVE <= ctx->shstrtab->chunk->shndx)
        *(u32 *)&(hdr->sh_link) = ctx->shstrtab->chunk->shndx;

    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if(chunk->shndx)
            hdr[chunk->shndx] = chunk->shdr;
    }
}

void phdr_copy_buf(Context *ctx,Chunk *chunk) {
    write_vector(ctx,ctx->buf + *(u32 *)&(chunk->shdr.sh_offset), ctx->phdr->phdrs);
}

void strtab_copy_buf(Context *ctx,Chunk *chunk) {
    u8 *buf = ctx->buf + *(u32 *)&(chunk->shdr.sh_offset);
    buf[0] = '\0';

    if (!ctx->arg.strip_all && !ctx->arg.retain_symbols_file.size)
        memcpy(buf + 1, "$a\0$t\0$d", 9);
}

void shstrtab_copy_buf(Context *ctx,Chunk *chunk) {
    u8 *base = ctx->buf + *(u32 *)&(chunk->shdr.sh_offset);
    base[0] = '\0';

    for (int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if (kind(chunk) != HEADER && chunk->name != NULL)
            write_string(base + *(u32 *)&(chunk->shdr.sh_name), chunk->name);
    }       
}

void out_sec_populate_symtab (Context *ctx,Chunk *chunk) {
    if (chunk->num_local_symtab == 0)
        return;
}

void populate_symtab(Context *ctx,Chunk *chunk) {
    if(chunk->is_outsec)
        out_sec_populate_symtab(ctx,chunk);
}
// If we create range extension thunks, we also synthesize symbols to mark
// the locations of thunks. Creating such symbols is optional, but it helps
// disassembling and/or debugging our output.
void output_sec_populate_symtab(Context *ctx,Chunk *chunk) {
    if (chunk->num_local_symtab == 0)
        return;
}

void symtab_copy_buf(Context *ctx,Chunk *chunk) {
    ElfSym *buf = (ElfSym *)(ctx->buf + *(u32 *)&(chunk->shdr.sh_offset));
    memset(buf, 0, sizeof(ElfSym));
    
    // Create section symbols
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if(chunk->shndx) {
            ElfSym *sym = &buf[chunk->shndx];
            memset(sym, 0, sizeof(ElfSym));

            sym->st_type = STT_SECTION;
            *(u32 *)&(sym->st_value) = *(u32 *)&(chunk->shdr.sh_addr);
            *(u16 *)&(sym->st_shndx) = chunk->shndx;
        }
    }

    // Populate linker-synthesized symbols
    for(int i = 0; i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        populate_symtab(ctx,chunk);
    }
    // Copy symbols from input files
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        obj_populate_symtab(ctx,file);
    }

}

i64 get_st_shndx(ELFSymbol *sym){
    SectionFragment *frag = get_frag(sym);
    if (frag)
        if (frag->is_alive)
        return frag->output_section->chunk->shndx;

    InputSection *isec = elfsym_get_input_section(sym);
    if (sym) {
        if (isec->is_alive)
            return isec->output_section->chunk->shndx;
        // else if (is_killed_by_icf())
        //     return isec->leader->output_section->shndx;
    }

    return SHN_UNDEF;
};

ElfSym to_output_esym(Context *ctx,ELFSymbol *sym, u32 st_name,
                         U32 *shn_xindex,ObjectFile *file) {
    ElfSym esym1;
    memset(&esym1, 0, sizeof(esym1));

    *(u32 *)&(esym1.st_name) = st_name;
    esym1.st_type = elfsym_get_type(file,sym->sym_idx);
    esym1.st_size = esym(&(file->inputfile),sym->sym_idx)->st_size;

    if (is_local(ctx,sym,file,sym->sym_idx))
        esym1.st_bind = STB_LOCAL;
    else if (sym->is_weak)
        esym1.st_bind = STB_WEAK;
    else if (sym->file->inputfile.is_dso)
        esym1.st_bind = STB_GLOBAL;
    else
        esym1.st_bind = esym(&(file->inputfile),sym->sym_idx)->st_bind;

    i64 shndx = -1;
    Chunk *osec = get_output_section(sym);
    SectionFragment *frag = get_frag(sym);
    // if (sym->file->inputfile.is_dso || is_undef(esym1)) {
    //     *(u16 *)&(esym1.st_shndx) = SHN_UNDEF;
    //     if (sym->is_canonical)
    //     *(u32 *)&(esym1.st_value) = get_plt_addr(ctx);
    // } else 
    if (osec) {
        // Linker-synthesized symbols _ehdr_start
        shndx = osec->shndx;
        *(u32 *)&(esym1.st_value) = elfsym_get_addr(ctx,0,sym);
    } else if (frag) {
        // Section fragment 
        shndx = frag->output_section->chunk->shndx;
        // *(u32 *)&(esym1.st_value) = elfsym_get_addr(ctx);
    } 
    else if (!elfsym_get_input_section(sym)) {
        // Absolute symbol  main.C
        *(u16 *)&(esym1.st_shndx) = SHN_ABS;
        *(u32 *)&(esym1.st_value) = elfsym_get_addr(ctx,0,sym);
    } else { 
        // $a main
        shndx = get_st_shndx(sym);
        esym1.st_visibility = sym->visibility;
        *(u32 *)&(esym1.st_value) = elfsym_get_addr(ctx, NO_PLT,sym);
    }
    // Symbol's st_shndx is only 16 bits wide, so we can't store a large
    // section index there. If the total number of sections is equal to
    // or greater than SHN_LORESERVE (= 65280), the real index is stored
    // to a SHT_SYMTAB_SHNDX section which contains a parallel array of
    // the symbol table.
    if (0 <= shndx && shndx < SHN_LORESERVE) {
        *(u16 *)&(esym1.st_shndx) = shndx;
    } else if (SHN_LORESERVE <= shndx) {
        assert(shn_xindex);
        *(u16 *)&(esym1.st_shndx) = SHN_XINDEX;
        *(u32 *)&(shn_xindex) = shndx;
    }

    return esym1;
}

void write_to(Context *ctx,u8 *buf,OutputSection *osec) {
    for(int i = 0;i < osec->member.size;i++) {
        // Copy section contents to an output file.
        InputSection *isec = osec->member.data[i];
        isec_write_to(ctx,buf + isec->offset,isec,i);

        u64 this_end = isec->offset + isec->sh_size;
        u64 next_start;
        if (i + 1 < osec->member.size)
            next_start = ((InputSection *)(osec->member.data[i + 1]))->offset;
        else
            next_start = *(u32 *)&(osec->chunk->shdr.sh_size);

        u8 *loc = buf + this_end;
        i64 size = next_start - this_end;

        if (*(u32 *)&(osec->chunk->shdr.sh_flags) & SHF_EXECINSTR) {
        for (i64 i = 0; i + sizeof(target.filler) <= size; i += sizeof(target.filler))
            memcpy(loc + i, target.filler, sizeof(target.filler));
        } else {
            memset(loc, 0, size);
        }
    }

    for(int i = 0;i < osec->thunks.size;i++) {
        Thunk *thunk = osec->thunks.data[i];
        thunk_copy_buf(ctx,thunk);
    }
}

void out_sec_copy_buf(Context *ctx,Chunk *chunk) {
    if (*(u32 *)&(chunk->shdr.sh_type) != SHT_NOBITS)
        write_to(ctx, ctx->buf + *(u32 *)&(chunk->shdr.sh_offset),chunk->outsec);
}

void got_copy_buf(Context *ctx,Chunk *chunk) {
    Word *buf = (Word *)(ctx->buf + *(u32 *)&(chunk->shdr.sh_offset));
    memset(buf,0,*(u32 *)&(chunk->shdr.sh_size));

    ElfRel *rel = (ElfRel *)(ctx->buf + *(u32 *)&(ctx->reldyn->chunk->shdr.sh_offset) + chunk->reldyn_offset);

    // 目前没有got_entry
}

void gotplt_copy_buf(Context *ctx,Chunk *chunk) {
    Word *buf = (Word *)(ctx->buf + *(u32 *)&(chunk->shdr.sh_offset));
    buf->val[0] = ctx->dynamic ? (u64)*(u32 *)&(ctx->dynamic->chunk->shdr.sh_addr) : 0;
    buf->val[1] = 0;
    buf->val[2] = 0;

    for (i64 i = 0; i < ctx->plt->symbols.size; i++)
        buf->val[i + 3] = *(u32 *)&(ctx->plt->chunk->shdr.sh_addr);
}
void merge_write_to(Context *ctx, u8 *buf,MergedSection *osec) {
    i64 shard_size = 256;

    // memset(buf + *(u32 *)&(osec->shard_offsets.data),0,*(u32 *)&(osec->shard_offsets.data));
    // for (i64 j = shard_size * i; j < shard_size * (i + 1); j++)
    // if (const char *key = map.entries[j].key) {
    //     SectionFragment *frag = osec->map map.entries[j].value;
    //     if (frag.is_alive)
    //     memcpy(buf + frag.offset, key, map.entries[j].keylen);
    // }
    // for(int j = 9;j < merge_string.size;j++) {
    //     SectionFragment *frag = merger_sec_find_element(osec->map,(char *)merge_string.data[j]);
    //     if (frag->is_alive)
    //         memcpy(buf + frag->offset, (char *)merge_string.data[j], sizeof(merge_string.data[j]));
    // }
}

void merged_sec_copy_buf(Context *ctx,Chunk *chunk) {
    merge_write_to(ctx, ctx->buf + *(u32 *)&(chunk->shdr.sh_offset),chunk->merge_sec);
}

void ehframe_hdr_copy_buf(Context *ctx,Chunk *chunk) {
    u8 *base = ctx->buf + *(u32 *)&(chunk->shdr.sh_offset);

    base[0] = 1;
    base[1] = DW_EH_PE_pcrel | DW_EH_PE_sdata4;
    base[2] = DW_EH_PE_udata4;
    base[3] = DW_EH_PE_datarel | DW_EH_PE_sdata4;

    *(u32 *)(base + 4) = *(u32 *)&(ctx->eh_frame->chunk->shdr.sh_addr) - *(u32 *)&(chunk->shdr.sh_addr) - 4;
    *(u32 *)(base + 8) = ctx->eh_frame_hdr->num_fdes;
}