#include "mold.h"
#include "xxhash.h"



char *get_output_name(Context *ctx, char *name, u64 flags) {
    if (ctx->arg.relocatable && !ctx->arg.relocatable_merge_sections)
        return name;
    // if (ctx->arg.unique && ctx->arg.unique->match(name))
    //     return name;
    if (flags & SHF_MERGE)
        return name;

    if (!strcmp(target.target_name,"arm32")) {
        if (starts_with(name,".ARM.exidx"))
            return ".ARM.exidx";
        if (starts_with(name,".ARM.extab"))
            return ".ARM.extab";
    }

    if (ctx->arg.z_keep_text_section_prefix) {
        char *prefixes[] = {
        ".text.hot.", ".text.unknown.", ".text.unlikely.", ".text.startup.",
        ".text.exit."
        };
        for (int i = 0;;i++) {
            char *stem = prefixes[i];
            if (stem == NULL) 
                break;
            if (!strcmp(name,stem) || starts_with(name,stem)) 
                return stem;
        }
    }

    static char *prefixes[] = {
        ".text.", ".data.rel.ro.", ".data.", ".rodata.", ".bss.rel.ro.", ".bss.",
        ".init_array.", ".fini_array.", ".tbss.", ".tdata.", ".gcc_except_table.",
        ".ctors.", ".dtors.", ".gnu.warning.", ".openbsd.randomdata.",
    };

    for (int i = 0;;i++) {
        char *stem = prefixes[i];
        if (stem == NULL) 
            break;
        if (!strcmp(name,stem) || starts_with(name,stem)) 
            return stem;
    }

    return name;
}
u64 canonicalize_type(char *name, u64 type) {
  if (type == SHT_PROGBITS) {
    if (!strcmp(name,".init_array") || starts_with(name,".init_array."))
      return SHT_INIT_ARRAY;
    if (!strcmp(name,".fini_array") || starts_with(name,".fini_array."))
      return SHT_FINI_ARRAY;
  }
  return type;
}
OutputSectionKey *get_output_section_key(Context *ctx,InputSection *isec,ObjectFile *file,int i) {
    const ElfShdr *shdr = get_shdr(file,i);
    char *name = strdup(get_output_name(ctx, get_name(file,i), *shdr->sh_flags.val));
    u64 type = canonicalize_type(name, *shdr->sh_type.val);
    u64 flags = *shdr->sh_flags.val & ~(u64)(SHF_COMPRESSED | SHF_GROUP | SHF_GNU_RETAIN);

    // .init_array is usually writable. We don't want to create multiple
    // .init_array output sections, so make it always writable.
    // So is .fini_array.
    if (type == SHT_INIT_ARRAY || type == SHT_FINI_ARRAY)
        flags |= SHF_WRITE;
    OutputSectionKey *key = (OutputSectionKey *)malloc(sizeof(OutputSectionKey));
    key->flags = flags;
    key->name = strdup(name);
    key->type = type;
    return key;
}
void create_internal_file(Context *ctx) {
    ctx->obj_pool = (ObjectFile **)malloc(sizeof(ObjectFile *) * 10);
    ObjectFile *obj = (ObjectFile *)malloc(sizeof(ObjectFile));
    int size = 0;
    add_object_to_pool(ctx->obj_pool, &size, obj);

    ctx->internal_obj = obj;


    VectorNew(&(ctx->internal_esyms), 1);
    ElfSym *elfsym;

    ELFSymbol *sym;
    sym_init(sym);

    VectorNew(&(obj->symbols), 1);
    VectorAdd(&(obj->symbols),sym,sizeof(ELFSymbol *));
    obj->inputfile.first_global = 1;
    obj->inputfile.is_alive = true;
    obj->inputfile.priority = 1;
    obj->inputfile.filename = strdup("<internal>");
    obj->inputfile.elf_syms = (ElfSym **)(ctx->internal_esyms.data);
    resize(&(obj->has_symver), ctx->internal_esyms.size);

    VectorAdd(&(ctx->objs),obj,sizeof(ObjectFile *));
    VectorAdd(&(ctx->internal_esyms),elfsym,sizeof(ElfSym *));

    ObjectFile *file = *((ObjectFile**)(ctx->objs.data) + 1);

}

void mark_live_objects(Context *ctx) {
    // for (Symbol<E> *sym : ctx.arg.undefined)
    //     if (sym->file)
    //         sym->file->is_alive = true;

    vector roots;
    Inputefile *input;
    VectorNew(&roots,1);
    

    for (int i = 0;;i++) {
        ObjectFile *file = (ObjectFile *)(ctx->objs.data[i]);
        if (file == NULL) {
            break;
        }
        if (file->inputfile.is_alive) {
            VectorAdd(&roots,file,sizeof(Inputefile *));
        }
    }
}

void do_resolve_symbols(Context *ctx) {
    mark_live_objects(ctx);

    // clear_symbols();
}

// 解析符号
void resolve_symbols(Context *ctx) {
    vector objs = ctx->objs;

    do_resolve_symbols(ctx);

}

char* save_string(Context *ctx, char *str, int len) {
    char *buf = (char *)malloc(len + 1);
    if (buf != NULL) {
        memcpy(buf, str, len);
        buf[len] = '\0';
        VectorNew(&(ctx->string_pool),1);
        VectorAdd(&(ctx->string_pool),buf,len + 1);
        return buf;
    } else {
        return NULL;
    }
}
uint64_t hash_string(char *str) {
    size_t size = strlen(str);
    return XXH3_64bits(str, size);
}

SectionFragment *SectionFragment_create(MergedSection *sec,bool is_alive) {
    SectionFragment *sf = (SectionFragment *) malloc(sizeof(SectionFragment));
    sf->output_section = sec;
    sf->offset = -1;
    sf->is_alive = is_alive;
    return sf;
}

// 插入到sec->map
void insert(MergedSection *sec,Context *ctx,char *data,u64 hash,i64 p2align) {
    bool is_alive = !ctx->arg.gc_sections || !(*sec->chunk->shdr.sh_flags.val & SHF_ALLOC);
    SectionFragment *frag = SectionFragment_create(sec,is_alive);
    merger_sec_insert_element(sec,data,frag/*,hash*/);
}

// 增加.comment段等
MergedSection *add_comment_string(Context *ctx, char *str) {
    MergedSection *sec = get_instance(ctx, ".comment", SHT_PROGBITS,
                                    SHF_MERGE | SHF_STRINGS, 1, 1);
    char * buf = save_string(ctx, str,strlen(str));
    // std::string_view data(buf.data(), buf.size() + 1);
    // sec->insert(ctx, data, hash_string(data), 0);
    u64 hash = hash_string(buf);
    insert(sec,ctx,buf,hash,0);
    return sec;
}

void compute_merged_section_sizes(Context *ctx) {
    MergedSection *sec;
    if (!ctx->arg.oformat_binary) {
        sec = add_comment_string(ctx, mold_version);
    }
    assign_offsets(ctx,sec);
}
bool sec_order_find (Context *ctx,char *name) {
    if (ctx->arg.section_order.size != 0) {
        for (int i = 0;i < ctx->arg.section_order.size;i++) {
            SectionOrder *ord = (SectionOrder *)(ctx->arg.section_order.data[i]);
            if (ord->type == SECTION && !strcmp(ord->name,name)) 
                return true;
        }
        return false;
    }
}

OutputEhdr *create_section_order_e(u32 sh_flags) {
    OutputEhdr *out_endr = (OutputEhdr *)malloc(sizeof(OutputEhdr));
    out_endr->chunk = (Chunk *)malloc(sizeof(Chunk));
    out_endr->chunk->name = strdup("EHDR");
    *out_endr->chunk->shdr.sh_flags.val = sh_flags;
    *out_endr->chunk->shdr.sh_size.val = sizeof(ElfEhdr);
    *out_endr->chunk->shdr.sh_addralign.val = sizeof(Word);
    return out_endr;
}

OutputPhdr *create_section_order_p(u32 sh_flags) {
    OutputPhdr *out_endr = (OutputPhdr *) malloc(sizeof(OutputPhdr));
    out_endr->chunk = (Chunk *)malloc(sizeof(Chunk));
    out_endr->chunk->name = strdup("PHDR");
    *out_endr->chunk->shdr.sh_size.val = 1;
    *out_endr->chunk->shdr.sh_addralign.val = sizeof(Word);
    return out_endr;
}

OutputShdr *create_section_order_s(u32 sh_flags) {
    OutputShdr *out_endr = (OutputShdr *)malloc(sizeof(OutputShdr)) ;
    out_endr->chunk = (Chunk *)malloc(sizeof(Chunk));
    out_endr->chunk->name = strdup("SHDR");
    *out_endr->chunk->shdr.sh_flags.val = sh_flags;
    *out_endr->chunk->shdr.sh_addralign.val = sizeof(Word);
    return out_endr;
}

GotSection *create_got(u32 sh_flags) {
    GotSection *got = (GotSection *)malloc(sizeof(GotSection));
    got->chunk = (Chunk *)malloc(sizeof(Chunk));
    got->chunk->name = strdup(".got");
    got->chunk->is_relro = true;
    *got->chunk->shdr.sh_type.val = SHT_PROGBITS;
    *got->chunk->shdr.sh_flags.val = SHF_ALLOC | SHF_WRITE;
    *got->chunk->shdr.sh_addralign.val  = sizeof(Word);
    *got->chunk->shdr.sh_size.val  = sizeof(Word);
    return got;
}

GotPltSection *create_got_plt(Context *ctx) {
    GotPltSection *got_plt = (GotPltSection *)malloc(sizeof(GotPltSection));
    got_plt->chunk = (Chunk *)malloc(sizeof(Chunk));
    got_plt->chunk->name = strdup(".got.plt");
    got_plt->chunk->is_relro = ctx->arg.z_now;
    *got_plt->chunk->shdr.sh_type.val = SHT_PROGBITS;
    *got_plt->chunk->shdr.sh_flags.val = SHF_ALLOC | SHF_WRITE;
    *got_plt->chunk->shdr.sh_addralign.val = sizeof(Word);
    got_plt->HDR_SIZE = 3 * sizeof(Word);
    got_plt->ENTRY_SIZE = sizeof(Word);
    *got_plt->chunk->shdr.sh_size.val = got_plt->HDR_SIZE;
    return got_plt;
}
RelDynSection *create_reldyn() {
    RelDynSection *reldyn = (RelDynSection *)malloc(sizeof(RelDynSection));
    reldyn->chunk = (Chunk *)malloc(sizeof(Chunk));
    reldyn->chunk->name = target.is_rela ? strdup(".rela.dyn") : strdup(".rel.dyn");
    *reldyn->chunk->shdr.sh_type.val = target.is_rela ? SHT_RELA : SHT_REL;
    *reldyn->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *reldyn->chunk->shdr.sh_entsize.val = sizeof(ElfRel);
    return reldyn;
}

RelPltSection *create_relplt() {
    RelPltSection *relplt = (RelPltSection *)malloc(sizeof(RelPltSection));
    relplt->chunk = (Chunk *)malloc(sizeof(Chunk));
    relplt->chunk->name = target.is_rela ? strdup(".rela.plt") : strdup(".rel.plt");
    *relplt->chunk->shdr.sh_type.val = target.is_rela ? SHT_RELA : SHT_REL;
    *relplt->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *relplt->chunk->shdr.sh_entsize.val =sizeof(ElfRel);
    *relplt->chunk->shdr.sh_addralign.val = sizeof(Word);
    return relplt;
}
StrtabSection *create_strtab() {
    StrtabSection *strtab = (StrtabSection *)malloc(sizeof(StrtabSection));
    strtab->chunk = (Chunk *)malloc(sizeof(Chunk));
    strtab->chunk->name = strdup(".strtab");
    *strtab->chunk->shdr.sh_type.val = SHT_STRTAB;
    strtab->ARM = 1;
    strtab->THUMB = 4;
    strtab->DATA = 7;
    return strtab;
}

PltSection *create_plt() {
    PltSection *plt = (PltSection *)malloc(sizeof(PltSection));
    plt->chunk = (Chunk *)malloc(sizeof(Chunk));
    plt->chunk->name = strdup(".plt");
    *plt->chunk->shdr.sh_type.val = SHT_PROGBITS;
    *plt->chunk->shdr.sh_flags.val = SHF_ALLOC | SHF_EXECINSTR;
    *plt->chunk->shdr.sh_addralign.val = 16;
    return plt;
}

PltGotSection *create_plt_got() {
    PltGotSection *plt_got = (PltGotSection *)malloc(sizeof(PltGotSection));
    plt_got->chunk = (Chunk *)malloc(sizeof(Chunk));
    plt_got->chunk->name = strdup(".plt.got");
    *plt_got->chunk->shdr.sh_type.val = SHT_PROGBITS;
    *plt_got->chunk->shdr.sh_flags.val = SHF_ALLOC | SHF_EXECINSTR;
    *plt_got->chunk->shdr.sh_addralign.val = 16;
    return plt_got;
}

SymtabSection *create_symtab() {
    SymtabSection *symtab = (SymtabSection *)malloc(sizeof(SymtabSection));
    symtab->chunk = (Chunk *)malloc(sizeof(Chunk));
    symtab->chunk->name = strdup(".symtab");
    *symtab->chunk->shdr.sh_type.val = SHT_SYMTAB;
    *symtab->chunk->shdr.sh_entsize.val = sizeof(ElfSym);
    *symtab->chunk->shdr.sh_addralign.val = sizeof(Word);
    return symtab;
}

DynsymSection *create_dynsym() {
    DynsymSection *dynsym = (DynsymSection *)malloc(sizeof(DynsymSection));
    dynsym->finalized = false;
    dynsym->chunk = (Chunk *)malloc(sizeof(Chunk));
    dynsym->chunk->name =  strdup(".symtab");
    *dynsym->chunk->shdr.sh_type.val = SHT_DYNSYM;
    *dynsym->chunk->shdr.sh_entsize.val = sizeof(ElfSym);
    *dynsym->chunk->shdr.sh_addralign.val = sizeof(Word);
    return dynsym;
}
EhFrameSection *create_enframe() {
    EhFrameSection *ehframe = (EhFrameSection *)malloc(sizeof(EhFrameSection));
    ehframe->chunk = (Chunk *)malloc(sizeof(Chunk));
    ehframe->chunk->name =  strdup(".eh_frame");
    *ehframe->chunk->shdr.sh_type.val = SHT_PROGBITS;
    *ehframe->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *ehframe->chunk->shdr.sh_addralign.val = sizeof(Word);
    return ehframe;
}

ShstrtabSection *create_shstrtab() {
    ShstrtabSection *shstrtab = (ShstrtabSection *)malloc(sizeof(ShstrtabSection));
    shstrtab->chunk = (Chunk *)malloc(sizeof(Chunk));
    shstrtab->chunk->name =  strdup(".shstrtab");
    *shstrtab->chunk->shdr.sh_type.val = SHT_STRTAB;
    return shstrtab;
}

// 创建特殊段
EhFrameHdrSection *create_ehframehdr() {
    EhFrameHdrSection *ehframehdr = (EhFrameHdrSection *)malloc(sizeof(EhFrameHdrSection));
    ehframehdr->chunk = (Chunk *)malloc(sizeof(Chunk));
    ehframehdr->chunk->name =  strdup(".eh_frame_hdr");
    *ehframehdr->chunk->shdr.sh_type.val = SHT_PROGBITS; 
    *ehframehdr->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *ehframehdr->chunk->shdr.sh_addralign.val = 4;
    ehframehdr->HEADER_SIZE = 12;
    ehframehdr->num_fdes = 0;
    *ehframehdr->chunk->shdr.sh_size.val = ehframehdr->HEADER_SIZE;
    return ehframehdr;
}

void create_synthetic_sections(Context *ctx) {
    if (ctx->arg.section_order.size == 0 || sec_order_find(ctx,"EHDR")) {
        ctx->ehdr = create_section_order_e(SHF_ALLOC);
        // VectorAdd(&(ctx->chunks),ctx->ehdr,sizeof(OutputEhdr *));
        VectorAdd(&(ctx->chunks),ctx->ehdr->chunk,sizeof(Chunk *));
    } else {
        ctx->ehdr = create_section_order_e(0);
        VectorAdd(&(ctx->chunks),ctx->ehdr->chunk,sizeof(Chunk *));
    }

    if (ctx->arg.section_order.size == 0 || sec_order_find(ctx,"PHDR")) {
        ctx->phdr = create_section_order_p(SHF_ALLOC);
        VectorAdd(&(ctx->chunks),ctx->phdr->chunk,sizeof(Chunk *));
    } else {
        ctx->phdr = create_section_order_p(0);
        VectorAdd(&(ctx->chunks),ctx->phdr->chunk,sizeof(Chunk *));
    }
    if (ctx->arg.z_sectionheader) {
        ctx->shdr = create_section_order_s(0);
        VectorAdd(&(ctx->chunks),ctx->shdr->chunk,sizeof(Chunk *));
    }

    ctx->got = create_got(0);
    VectorAdd(&(ctx->chunks),ctx->got->chunk,sizeof(Chunk *));
    ctx->gotplt = create_got_plt(ctx);
    VectorAdd(&(ctx->chunks),ctx->gotplt->chunk,sizeof(Chunk *));

    ctx->reldyn = create_reldyn();
    VectorAdd(&(ctx->chunks),ctx->reldyn->chunk,sizeof(Chunk *));
    ctx->relplt = create_relplt();
    VectorAdd(&(ctx->chunks),ctx->relplt->chunk,sizeof(Chunk *));

    ctx->strtab = create_strtab();
    VectorAdd(&(ctx->chunks),ctx->strtab->chunk,sizeof(Chunk *));

    ctx->plt = create_plt();
    VectorAdd(&(ctx->chunks),ctx->plt->chunk,sizeof(Chunk *));

    ctx->pltgot = create_plt_got();
    VectorAdd(&(ctx->chunks),ctx->pltgot->chunk,sizeof(Chunk *));
    ctx->symtab = create_symtab();
    VectorAdd(&(ctx->chunks),ctx->symtab->chunk,sizeof(Chunk *));

    ctx->dynsym = create_dynsym();
    VectorAdd(&(ctx->chunks),ctx->dynsym->chunk,sizeof(Chunk *));

    ctx->eh_frame = create_enframe();
    VectorAdd(&(ctx->chunks),ctx->eh_frame->chunk,sizeof(Chunk *));

    if (ctx->shdr) {
        ctx->shstrtab = create_shstrtab();
        VectorAdd(&(ctx->chunks),ctx->shdr->chunk,sizeof(Chunk *));
    }
    if (ctx->arg.eh_frame_hdr) {
        ctx->eh_frame_hdr = create_ehframehdr();
        VectorAdd(&(ctx->chunks),ctx->eh_frame_hdr->chunk,sizeof(Chunk *));
    }
    if (ctx->arg.z_relro && ctx->arg.section_order.size == 0 &&
        ctx->arg.z_separate_code != SEPARATE_LOADABLE_SEGMENTS) {

    }
}

OutputSection *create_a_output_sections(Context *ctx,char *name,u32 type, u64 flags) {
    OutputSection *output_section = (OutputSection *)malloc(sizeof(OutputSection));
    output_section->chunk = (Chunk *)malloc(sizeof(Chunk));
    output_section->chunk->name = strdup(name);
    *output_section->chunk->shdr.sh_type.val = type;
    *output_section->chunk->shdr.sh_flags.val = flags & ~SHF_MERGE & ~SHF_STRINGS;
    return output_section;
}

// 比较函数，用于排序
int compareChunks(const void* a, const void* b) {
    Chunk* chunkA = (Chunk* )a;
    Chunk* chunkB = (Chunk* )b;
    if (chunkA->name && chunkB->name) {
        // 按照指定的排序规则进行比较
        int nameComparison = strcmp(chunkA->name, chunkB->name);
        if (nameComparison != 0) 
            return nameComparison;
    }
    
    
    if (*chunkA->shdr.sh_type.val <  *chunkB->shdr.sh_type.val) 
        return -1;
    else 
        return 1;
    if (*chunkA->shdr.sh_flags.val < *chunkB->shdr.sh_flags.val) 
        return -1;
    else 
        return 1;
    
    return 0;
}
// 创造输出段 Create output sections for input sections.
void create_output_sections(Context *ctx) {
    i64 size = ctx->osec_pool.size;
    OutputSection_element *output_section_map = NULL;
    // // Instantiate output sections
    for (int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        for (int j = 0;;j++) {
            if (file == NULL || file->sections == NULL) {
                break;
            }
            if (file->sections[j] == NULL) {
                break;
            }
            InputSection *isec = (InputSection *)malloc(sizeof(InputSection));
            isec = file->sections[j];  
            if (!isec || !isec->is_alive || !isec->is_constucted)
                continue;   
            ElfShdr *shdr = get_shdr(file,j);
            if (ctx->arg.relocatable && (*shdr->sh_flags.val & SHF_GROUP)) {
                OutputSection *osec = create_a_output_sections(ctx, get_name(file,j), *shdr->sh_type.val, *shdr->sh_flags.val);
                isec->output_section = osec;
                VectorAdd(&(ctx->osec_pool),osec,sizeof(OutputSection));
                continue;
            }
            OutputSectionKey *key = get_output_section_key(ctx, isec,file,j);
            OutputSection *osec = create_a_output_sections(ctx, key->name, key->type, key->flags);
                
            out_sec_insert_element(output_section_map,key,osec);
            VectorAdd(&(ctx->osec_pool),osec,sizeof(OutputSection));
            isec->output_section = osec;
        }
    }
    // Add input sections to output sections
    vector chunks;
    VectorNew(&chunks,1);
    for(int i = 0;i < ctx->osec_pool.size;i++) {
        OutputSection *temp = ctx->osec_pool.data[i];
        VectorAdd(&chunks,temp->chunk,sizeof(Chunk *));
    }

    for (int i = 0;;i++) {
        ObjectFile *file = (ObjectFile *)malloc(sizeof(ObjectFile));
        file = ctx->objs.data[i];
        if (file == NULL)
            break;
        for (int j = 0;;j++) {
            if (file->sections == NULL)
                break;
            InputSection *isec = file->sections[j];
            if (isec == NULL)
                break;
            isec->output_section = (OutputSection *)malloc(sizeof(OutputSection));
            VectorNew(&(isec->output_section->member),1);
            if (isec && isec->is_alive)
                VectorAdd(&(isec->output_section->member),isec,sizeof(InputSection *));
        }    
    }

    // Add output sections and mergeable sections to ctx.chunks
    for (int i = 0;i < ctx->merged_sections_count;i++) {
        MergedSection *osec = ctx->merged_sections.data[i];
        if (*osec->chunk->shdr.sh_size.val) {
            VectorAdd(&chunks,osec->chunk,sizeof(Chunk *));
        }
    }
    
    // 使用 qsort 函数对chunks进行排序
    Chunk temp1[chunks.size];
    for (int i = 0; i < chunks.size; i++) {
        temp1[i] = *(Chunk *)(chunks.data[i]);
    }
    
    qsort(temp1, chunks.size, sizeof(Chunk), compareChunks);
    
    for (int i = 0; i < chunks.size; i++) {
        // 获取当前元素
        Chunk* currentElement = &temp1[i];
        printf("name: %s\n", currentElement->name);
        // 将chunks加入ctx->chunks
        VectorAdd(&(ctx->chunks),currentElement,(sizeof(Chunk *)));
    } 
}

void resolve_section_pieces(Context *ctx) {
    for(int i = 0;;i++) {
        ObjectFile *file = ctx->objs.data[i];
        if (file == NULL) 
            break;
        initialize_mergeable_sections(ctx,file);
    }
    for(int i = 0;;i++) {
        ObjectFile *file = ctx->objs.data[i];
        if (file == NULL) 
            break;
        file_resolve_section_pieces(ctx,file);
    }
}