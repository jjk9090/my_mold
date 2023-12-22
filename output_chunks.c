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
        VectorNew(&ctx->merged_sections,1);
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
        printf("%ld\n",*((i64 *)(sec->shard_offsets.data[i])));
    }
    i = 0;
    printf("%ld\n",*((i64 *)(sec->shard_offsets.data[2])));
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
    chunk->num_local_symtab = chunk->pltsec->symbols.size;
    chunk->strtab_size = 0;
    for(int i = 0;i < chunk->pltgotsec->symbols.size;i++) {
        ELFSymbol *sym = chunk->pltsec->symbols.data[i];
        chunk->strtab_size += sym->namelen + sizeof("$pltgot");
    }
    if (!strcmp(target.target_name,"arm32"))
        chunk->num_local_symtab += chunk->pltsec->symbols.size * 2;
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
    return *chunk->shdr.sh_type.val == SHT_NOBITS;
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
    *phdr->p_type.val = type;
    *phdr->p_flags.val = flags;
    *phdr->p_align.val = *chunk->shdr.sh_addralign.val;
    *phdr->p_offset.val = *chunk->shdr.sh_offset.val;

    if (*chunk->shdr.sh_type.val != SHT_NOBITS)
        *phdr->p_filesz.val = *chunk->shdr.sh_size.val;

    *phdr->p_vaddr.val = *chunk->shdr.sh_addr.val;
    *phdr->p_paddr.val = *chunk->shdr.sh_addr.val;

    if (*chunk->shdr.sh_flags.val & SHF_ALLOC)
        *phdr->p_memsz.val = *chunk->shdr.sh_size.val;
    VectorAdd(vec,phdr,sizeof(ElfPhdr *));
}

bool is_note(Chunk *chunk) {
    return *chunk->shdr.sh_type.val == SHT_NOTE;
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
    *phdr->p_align.val = *phdr->p_align.val > *chunk->shdr.sh_addralign.val ? *phdr->p_align.val : *chunk->shdr.sh_addralign.val;
    *phdr->p_memsz.val = *chunk->shdr.sh_addr.val + *chunk->shdr.sh_size.val - *phdr->p_vaddr.val;
    if (*chunk->shdr.sh_type.val != SHT_NOBITS)
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
    // if (ctx->interp)
    //     define(PT_INTERP, PF_R, ctx->interp);

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
        Chunk *first = chunks.data[i++];
        i64 flags = to_phdr_flags(ctx,first);
        define(PT_LOAD, flags, first,&vec);

        // i32 s = *((ElfPhdr *)(vec.data[vec.size - 1]))->p_align.val;
        *((ElfPhdr *)(vec.data[vec.size - 1]))->p_align.val = 
                ctx->page_size > *((ElfPhdr *)(vec.data[vec.size - 1]))->p_align.val 
                ? ctx->page_size 
                : *((ElfPhdr *)(vec.data[vec.size - 1]))->p_align.val;
        
        if (!is_bss(first))
            while (i < chunks.size &&
                    !is_bss(chunks.data[i]) &&
                    to_phdr_flags(ctx, chunks.data[i]) == flags &&
                    *((Chunk *)(chunks.data[i]))->shdr.sh_offset.val - *first->shdr.sh_offset.val ==
                    *((Chunk *)(chunks.data[i]))->shdr.sh_addr.val - *first->shdr.sh_addr.val)
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
        phdr->p_type.val[0] = (PT_GNU_STACK >> 24) & 0xFF;
        phdr->p_type.val[1] = (PT_GNU_STACK >> 16) & 0xFF;
        phdr->p_type.val[2] = (PT_GNU_STACK >> 8) & 0xFF;
        phdr->p_type.val[3] = PT_GNU_STACK & 0xFF;
        *phdr->p_flags.val = ctx->arg.z_execstack ? (PF_R | PF_W | PF_X) : (PF_R | PF_W);
        *phdr->p_memsz.val = ctx->arg.z_stack_size;
        *phdr->p_align.val = 1;
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
                *((ElfPhdr *)vec.data[vec.size - 1])->p_align.val = 1;
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
    *chunk->shdr.sh_size.val = ctx->phdr->phdrs.size * sizeof(ElfPhdr);

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
    *chunk->shdr.sh_size.val = (offset == 1) ? 0 : offset;
}

ChunkKind kind(Chunk *chunk) {
    if (chunk->is_outsec)
        return OUTPUT_SECTION;
    return SYNTHETIC;
}

void shstrtab_update_shdr(Context *ctx,Chunk *chunk) {
    i64 offset = 1;
    for(int i = 0;i < ctx->chunks.size;i++) {
        *chunk->shdr.sh_name.val = offset;
        offset += strlen(chunk->name) + 1;
    }
    *chunk->shdr.sh_size.val = offset;
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
    for(int i = 0;i < ctx->objs.size;i++) {
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

    *chunk->shdr.sh_info.val = ((ObjectFile *)ctx->objs.data[0])->inputfile.global_symtab_idx;
    *chunk->shdr.sh_link.val = ctx->strtab->chunk->shndx;
    *chunk->shdr.sh_size.val = (nsyms == 1) ? 0 : nsyms * sizeof(ElfSym);
}

void gotplt_update_shdr(Context *ctx,Chunk *chunk) {
    *chunk->shdr.sh_size.val = ctx->gotplt->HDR_SIZE + ctx->plt->symbols.size * ctx->gotplt->ENTRY_SIZE;
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
    *chunk->shdr.sh_link.val = ctx->dynstr->chunk->shndx;
    *chunk->shdr.sh_size.val = sizeof(ElfSym) * ctx->dynsym->symbols.size;
}

void hash_update_shdr(Context *ctx,Chunk *chunk) {
    if (ctx->dynsym->symbols.size == 0)
        return;
    i64 header_size = 8;
    i64 num_slots = ctx->dynsym->symbols.size;
    *chunk->shdr.sh_size.val = header_size + num_slots * 8;
    *chunk->shdr.sh_link.val = ctx->dynsym->chunk->shndx;
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
