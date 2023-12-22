#include "mold.h"
#include "xxhash.h"
Context *global_ctx;

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
    ElfSym *elfsym = (ElfSym *)malloc(sizeof(ElfSym));

    ELFSymbol *sym = (ELFSymbol *)malloc(sizeof(ELFSymbol));
    sym_init(sym);

    VectorNew(&(obj->symbols), 1);
    VectorAdd(&(obj->symbols),sym,sizeof(ELFSymbol *));
    obj->inputfile.first_global = 1;
    obj->inputfile.is_alive = true;
    obj->inputfile.priority = 1;
    obj->inputfile.filename = strdup("<internal>");
    obj->inputfile.elf_syms = (ElfSym **)(ctx->internal_esyms.data);
    resize(&(obj->has_symver), ctx->internal_esyms.size);

    VectorNew(&(ctx->objs),1);
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
    for(int i = 0;;i++) {
        ObjectFile *file = (ObjectFile *)ctx->objs.data[i];
        if (file == NULL)
            break;
        obj_resolve_symbols(ctx,file);
    }
    mark_live_objects(ctx);

    // clear_symbols();
}

// 解析符号
void resolve_symbols(Context *ctx) {
    vector objs = ctx->objs;

    do_resolve_symbols(ctx);

}

char* save_string(Context *ctx, char *str, int len) {
    if (str) {
        VectorAdd(&(ctx->string_pool),str,sizeof(char *));
        return str;
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
    // *out_endr->chunk->shdr.sh_size.val = 1;
    *out_endr->chunk->shdr.sh_flags.val = sh_flags;
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
    dynsym->chunk->name =  strdup(".dynsym");
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

HashSection *create_hash() {
    HashSection *hash = (HashSection *)malloc(sizeof(HashSection));
    hash->chunk = (Chunk *)malloc(sizeof(Chunk));
    hash->chunk->name = strdup(".hash");
    *hash->chunk->shdr.sh_type.val = SHT_HASH;
    *hash->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *hash->chunk->shdr.sh_entsize.val = 4;
    *hash->chunk->shdr.sh_addralign.val = 4;
    return hash;
}

GnuHashSection *create_gnu_hash() {
    GnuHashSection *gnu_hash = (GnuHashSection *)malloc(sizeof(GnuHashSection));
    gnu_hash->chunk = (Chunk *)malloc(sizeof(Chunk));
    gnu_hash->chunk->name = strdup(".gnu.hash");
    // *gnu_hash->chunk->shdr.sh_type.val = SHT_GNU_HASH;
    gnu_hash->chunk->shdr.sh_type.val[0] = (SHT_GNU_HASH >> 24) & 0xFF;
    gnu_hash->chunk->shdr.sh_type.val[1] = (SHT_GNU_HASH >> 16) & 0xFF;
    gnu_hash->chunk->shdr.sh_type.val[2] = (SHT_GNU_HASH >> 8) & 0xFF;
    gnu_hash->chunk->shdr.sh_type.val[3] = SHT_GNU_HASH & 0xFF;
    *gnu_hash->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *gnu_hash->chunk->shdr.sh_addralign.val = sizeof(Word);
    gnu_hash->BLOOM_SHIFT = 26;
    gnu_hash->HEADER_SIZE = 16;
    gnu_hash->LOAD_FACTOR = 8;
    gnu_hash->num_bloom = 1;
    gnu_hash->num_buckets = -1;
    return gnu_hash;
}

DynstrSection *create_dynstr() {
    DynstrSection *dynstr = (DynstrSection *)malloc(sizeof(DynstrSection));
    dynstr->chunk = (Chunk *)malloc(sizeof(Chunk));
    dynstr->chunk->name = strdup(".dynstr");
    *dynstr->chunk->shdr.sh_type.val = SHT_STRTAB;
    *dynstr->chunk->shdr.sh_flags.val = SHF_ALLOC;
    dynstr->dynsym_offset = -1;
    return dynstr;
}

VersymSection *create_gnu_version() {
    VersymSection *gnu_version = (VersymSection *)malloc(sizeof(VersymSection));
    gnu_version->chunk = (Chunk *)malloc(sizeof(Chunk));
    gnu_version->chunk->name = strdup(".gnu.version");
    // *gnu_version->chunk->shdr.sh_type.val = SHT_GNU_VERSYM;
    gnu_version->chunk->shdr.sh_type.val[0] = (SHT_GNU_VERSYM >> 24) & 0xFF;
    gnu_version->chunk->shdr.sh_type.val[1] = (SHT_GNU_VERSYM >> 16) & 0xFF;
    gnu_version->chunk->shdr.sh_type.val[2] = (SHT_GNU_VERSYM >> 8) & 0xFF;
    gnu_version->chunk->shdr.sh_type.val[3] = SHT_GNU_VERSYM & 0xFF;
    *gnu_version->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *gnu_version->chunk->shdr.sh_entsize.val = 2;
    *gnu_version->chunk->shdr.sh_addralign.val = 2;
    VectorNew(&(gnu_version->contents),1);
    return gnu_version;
}

VerneedSection *create_verneed() {
    VerneedSection *verneed = (VerneedSection *)malloc(sizeof(VerneedSection));
    verneed->chunk = (Chunk *)malloc(sizeof(Chunk));
    verneed->chunk->name = strdup(".gnu.version_r");
    // *verneed->chunk->shdr.sh_type.val = SHT_GNU_VERNEED;
    verneed->chunk->shdr.sh_type.val[0] = (SHT_GNU_VERNEED >> 24) & 0xFF;
    verneed->chunk->shdr.sh_type.val[1] = (SHT_GNU_VERNEED >> 16) & 0xFF;
    verneed->chunk->shdr.sh_type.val[2] = (SHT_GNU_VERNEED >> 8) & 0xFF;
    verneed->chunk->shdr.sh_type.val[3] = SHT_GNU_VERNEED & 0xFF;
    *verneed->chunk->shdr.sh_flags.val = SHF_ALLOC;
    *verneed->chunk->shdr.sh_addralign.val = sizeof(Word);
    VectorNew(&(verneed->contents),1);
    return verneed;
}

RelroPaddingSection *create_relropadding() {
    RelroPaddingSection *relro = (RelroPaddingSection *)malloc(sizeof(RelroPaddingSection));
    relro->chunk = (Chunk *)malloc(sizeof(Chunk));
    relro->chunk->name = strdup(".relro_padding");
    relro->chunk->is_relro = true;
    *relro->chunk->shdr.sh_type.val = SHT_NOBITS;
    *relro->chunk->shdr.sh_flags.val = SHF_ALLOC | SHF_WRITE;
    *relro->chunk->shdr.sh_addralign.val = 1;
    *relro->chunk->shdr.sh_size.val = 1;
    return relro;
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
    ctx->got->chunk->gotsec = ctx->got;
    ctx->got->chunk->is_gotsec = true;
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
    ctx->plt->chunk->pltsec = ctx->plt;
    ctx->plt->chunk->is_plt = true;
    VectorAdd(&(ctx->chunks),ctx->plt->chunk,sizeof(Chunk *));

    ctx->pltgot = create_plt_got();
    ctx->pltgot->chunk->pltgotsec = ctx->pltgot;
    ctx->pltgot->chunk->is_pltgot = true;
    VectorAdd(&(ctx->chunks),ctx->pltgot->chunk,sizeof(Chunk *));
    
    ctx->symtab = create_symtab();
    VectorAdd(&(ctx->chunks),ctx->symtab->chunk,sizeof(Chunk *));

    ctx->dynsym = create_dynsym();
    VectorAdd(&(ctx->chunks),ctx->dynsym->chunk,sizeof(Chunk *));
    
    ctx->dynstr = create_dynstr();
    VectorAdd(&(ctx->chunks),ctx->dynstr->chunk,sizeof(Chunk *));

    ctx->eh_frame = create_enframe();
    VectorAdd(&(ctx->chunks),ctx->eh_frame->chunk,sizeof(Chunk *));

    if (ctx->shdr) {
        ctx->shstrtab = create_shstrtab();
        VectorAdd(&(ctx->chunks),ctx->shstrtab->chunk,sizeof(Chunk *));
    }
    if (ctx->arg.eh_frame_hdr) {
        ctx->eh_frame_hdr = create_ehframehdr();
        VectorAdd(&(ctx->chunks),ctx->eh_frame_hdr->chunk,sizeof(Chunk *));
    }
    if (ctx->arg.z_relro && ctx->arg.section_order.size == 0 &&
        ctx->arg.z_separate_code != SEPARATE_LOADABLE_SEGMENTS) {

    }
    if (ctx->arg.hash_style_sysv) {
        ctx->hash = create_hash();
        VectorAdd(&(ctx->chunks),ctx->hash->chunk,sizeof(Chunk *));
    }
        
    if (ctx->arg.hash_style_gnu) {
        ctx->gnu_hash = create_gnu_hash();
        VectorAdd(&(ctx->chunks),ctx->gnu_hash->chunk,sizeof(Chunk *));
    }
    if (ctx->arg.z_relro && ctx->arg.section_order.size == 0 &&
        ctx->arg.z_separate_code != SEPARATE_LOADABLE_SEGMENTS) {
        ctx->relro_padding = create_relropadding();
        VectorAdd(&(ctx->chunks),ctx->relro_padding->chunk,sizeof(Chunk *));
    }

    ctx->versym = create_gnu_version();
    VectorAdd(&(ctx->chunks),ctx->versym->chunk,sizeof(Chunk *));
    ctx->verneed = create_verneed();
    VectorAdd(&(ctx->chunks),ctx->verneed->chunk,sizeof(Chunk *));
}

OutputSection *create_a_output_sections(Context *ctx,char *name,u32 type, u64 flags) {
    OutputSection *output_section = (OutputSection *)malloc(sizeof(OutputSection));
    output_section->chunk = (Chunk *)malloc(sizeof(Chunk));
    output_section->chunk->name = strdup(name);
    *output_section->chunk->shdr.sh_type.val = type;
    *output_section->chunk->shdr.sh_flags.val = flags & ~SHF_MERGE & ~SHF_STRINGS;
    output_section->chunk->shndx = 0;
    output_section->chunk->is_relro = false;
    output_section->chunk->local_symtab_idx = 0;
    output_section->chunk->num_local_symtab = 0;
    output_section->chunk->strtab_size = 0;
    output_section->chunk->strtab_offset = 0;
    return output_section;
}

// 比较函数，用于排序 0x555555578150
int compareChunks(const void* a, const void* b) {
    Chunk* chunkA = *(Chunk**)a;
    Chunk* chunkB = *(Chunk**)b;
    if (chunkA->name && chunkB->name) {
        // 按照指定的排序规则进行比较
        int nameComparison = strcmp(chunkA->name, chunkB->name);
        if (nameComparison != 0) 
            return nameComparison;
    }
    
    
    if (*chunkA->shdr.sh_type.val <  *chunkB->shdr.sh_type.val) 
        return -1;
    else if (*chunkA->shdr.sh_type.val >  *chunkB->shdr.sh_type.val)
        return 1;
    if (*chunkA->shdr.sh_flags.val < *chunkB->shdr.sh_flags.val) 
        return -1;
    else if (*chunkA->shdr.sh_flags.val > *chunkB->shdr.sh_flags.val) 
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
                VectorAdd(&(ctx->osec_pool),osec,sizeof(OutputSection *));
                continue;
            }
            OutputSectionKey *key = get_output_section_key(ctx, isec,file,j);
            OutputSection *osec = create_a_output_sections(ctx, key->name, key->type, key->flags);
                
            out_sec_insert_element(output_section_map,key,osec);
            VectorAdd(&(ctx->osec_pool),osec,sizeof(OutputSection *));
            isec->output_section = osec;
            printf("wait ...\n");
        }
    }
    // Add input sections to output sections
    vector chunks;
    VectorNew(&chunks,1);
    for(int i = 0;i < ctx->osec_pool.size;i++) {
        OutputSection *temp = ctx->osec_pool.data[i];
        temp->chunk->is_outsec = 1;
        temp->chunk->outsec = (OutputSection *)malloc(sizeof(OutputSection));
        temp->chunk->outsec = temp;
        VectorAdd(&chunks,temp->chunk,sizeof(Chunk *));
        // VectorAdd(&(ctx->chunks),temp->chunk,sizeof(Chunk *));
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
            // isec->output_section = (OutputSection *)malloc(sizeof(OutputSection));
            if (isec && isec->is_alive && isec->file != NULL) {
                VectorNew(&(isec->output_section->member),1);
                VectorAdd(&(isec->output_section->member),isec,sizeof(InputSection *));
            }
        }    
    }

    // Add output sections and mergeable sections to ctx.chunks
    for (int i = 0;i < ctx->merged_sections_count;i++) {
        MergedSection *osec = ctx->merged_sections.data[i];
        if (*osec->chunk->shdr.sh_size.val) {
            VectorAdd(&chunks,osec->chunk,sizeof(Chunk *));
            // VectorAdd(&(ctx->chunks),osec->chunk,sizeof(Chunk *));
        }
    }
    
    // 使用 qsort 函数对chunks进行排序
    Chunk *temp2[chunks.size];
    for (int i = 0; i < chunks.size; i++) {
        temp2[i] = (Chunk *)(chunks.data[i]);
    }
    qsort(temp2, chunks.size, sizeof(Chunk *), compareChunks);
    for (int i = 0; i < chunks.size; i++) {
        // 获取当前元素
        Chunk* currentElement = temp2[i];
        printf("name: %s\n", currentElement->name);
        // 将chunks加入ctx->chunks
        // VectorAdd(&(ctx->chunks),ctx->phdr->chunk,sizeof(Chunk *));
        VectorAdd(&(ctx->chunks),temp2[i],(sizeof(Chunk *)));
    } 
    
    for (int i = 0; i < ctx->chunks.size; i++) {
        // 获取当前元素
        Chunk* currentElement = ctx->chunks.data[i];
        printf("name: %s\n", currentElement->name);
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

ELFSymbol *add_sym(Context *ctx,ObjectFile *obj,char *name) {
    u32 type = STT_NOTYPE;
    ElfSym *esym = (ElfSym *)malloc(sizeof(ElfSym));
    esym->st_type = type;
    // *esym->st_shndx.val = SHN_ABS;
    esym->st_shndx.val[0] = (SHN_ABS >> 8) & 0xFF;
    esym->st_shndx.val[1] = SHN_ABS & 0xFF;
    esym->st_bind = STB_GLOBAL;
    esym->st_visibility = STV_HIDDEN;
    VectorAdd(&(ctx->internal_esyms),esym,sizeof(ElfSym *));
    ELFSymbol *sym = insert_symbol(ctx,name,name);
    sym_init(sym);
    sym->value =  0xdeadbeef; // unique dummy value
    VectorAdd(&(obj->symbols),sym,sizeof(ELFSymbol *));
    return sym;
}

char* get_start_stop_name(Context* ctx, Chunk* chunk) {
    if ((*chunk->shdr.sh_flags.val & SHF_ALLOC) && strlen(chunk->name) > 0) {
        if (is_c_identifier(chunk->name)) {
            char* result = (char*)malloc(strlen(chunk->name) + 1);
            strcpy(result, chunk->name);
            return result;
        }

        if (ctx->arg.start_stop) {
            char* s = (char*)malloc(strlen(chunk->name) + 1);
            strcpy(s, chunk->name);
            if (s[0] == '.')
                memmove(s, s + 1, strlen(s));
            for (size_t i = 0; i < strlen(s); i++) {
                if (!is_alnum(s[i]))
                    s[i] = '_';
            }
            return s;
        }
    }

    return NULL;
}

// 增加合成符号
void add_synthetic_symbols (Context *ctx) {
    ObjectFile *obj = ctx->internal_obj;
    ctx->__ehdr_start = add_sym(ctx,obj,"__ehdr_start");
    ctx->__init_array_start = add_sym(ctx,obj,"__init_array_start");

    ctx->__init_array_end = add_sym(ctx,obj,"__init_array_end");
    ctx->__fini_array_start = add_sym(ctx,obj,"__fini_array_start");
    ctx->__fini_array_end = add_sym(ctx,obj,"__fini_array_end");
    ctx->__preinit_array_start = add_sym(ctx,obj,"__preinit_array_start");
    ctx->__preinit_array_end = add_sym(ctx,obj,"__preinit_array_end");
    ctx->_DYNAMIC = add_sym(ctx,obj,"_DYNAMIC");
    ctx->_GLOBAL_OFFSET_TABLE_ = add_sym(ctx,obj,"_GLOBAL_OFFSET_TABLE_");
    ctx->_PROCEDURE_LINKAGE_TABLE_ = add_sym(ctx,obj,"_PROCEDURE_LINKAGE_TABLE_");
    ctx->__bss_start = add_sym(ctx,obj,"__bss_start");
    ctx->_end = add_sym(ctx,obj,"_end");
    ctx->_etext = add_sym(ctx,obj,"_etext");
    ctx->_edata = add_sym(ctx,obj,"_edata");
    ctx->__executable_start = add_sym(ctx,obj,"__executable_start");

    ctx->__rel_iplt_start =
        add_sym(ctx,obj,target.is_rela ? "__rela_iplt_start" : "__rel_iplt_start");
    ctx->__rel_iplt_end =
        add_sym(ctx,obj,target.is_rela ? "__rela_iplt_end" : "__rel_iplt_end");

    if (ctx->arg.eh_frame_hdr)
        ctx->__GNU_EH_FRAME_HDR = add_sym(ctx,obj,"__GNU_EH_FRAME_HDR");
    
    if (!insert_symbol(ctx, "end","end")->file)
        ctx->end = add_sym(ctx,obj,"end");
    if (!insert_symbol(ctx, "etext","etext")->file)
        ctx->etext = add_sym(ctx,obj,"etext");
    if (!insert_symbol(ctx, "edata","edata")->file)
        ctx->edata = add_sym(ctx,obj,"edata");
    if (!insert_symbol(ctx, "__dso_handle","__dso_handle")->file)
        ctx->__dso_handle = add_sym(ctx,obj,"__dso_handle");
    
    if (!strcmp(target.target_name,"arm32")) {
        ctx->__exidx_start = add_sym(ctx,obj,"__exidx_start");
        ctx->__exidx_end = add_sym(ctx,obj,"__exidx_end");
    }
    
    for (int i = 0; i < ctx->chunks.size; i++) {        
        Chunk *chunk = (Chunk *)ctx->chunks.data[i];
        
        char *right = get_start_stop_name(ctx,chunk);
        if (right) {
            char temp[60];
            strcpy(temp, "__start_");
            strcat(temp, right);
            char *s1 = strdup(temp);
            add_sym(ctx, obj, s1);

            strcpy(temp, "__stop_");
            strcat(temp, right);
            char *s2 = strdup(temp);
            add_sym(ctx, obj, s2);
        }
    }

    int size = ctx->internal_esyms.size;
    obj->inputfile.elf_syms = (ElfSym **)malloc(sizeof(ElfSym *) * size);
    // obj->inputfile.elf_syms = ctx->internal_esyms;
    for(int i = 0;i < size;i++) {
        ElfSym *temp = (ElfSym *)ctx->internal_esyms.data[i];
        obj->inputfile.elf_syms[i] = (ElfSym *)ctx->internal_esyms.data[i];
    }
    resize(&(obj->has_symver),size - 1);
    obj_resolve_symbols(ctx,obj);

    // Make all synthetic symbols relative ones by associating them to
    // a dummy output section.
    for(int i = 0;i < obj->symbols.size;i++) {
        ELFSymbol *sym = (ELFSymbol *)(obj->symbols.data[i]);
        if (sym->file == obj) 
            set_output_section(ctx->symtab->chunk,sym);
    }
}

static vector split(vector input, i64 unit) {
    vector vec;
    VectorNew(&vec, 1);
    void** data = (void**)input.data;
    int remaining = input.size;
    while (remaining >= unit) {
        vector tmp;
        VectorNew(&tmp, unit);
        memcpy(tmp.data, data, unit * sizeof(void*));
        VectorAdd(&vec, &tmp, sizeof(vector*));
        data += unit;
        remaining -= unit;
    }
    if (remaining > 0) {
        vector tmp;
        VectorNew(&tmp, remaining);
        memcpy(tmp.data, data, remaining * sizeof(void*));
        VectorAdd(&vec, &tmp, sizeof(vector*));
    }
    return vec;
}

typedef struct {
    i64 size;
    i64 p2align;
    i64 offset;
    vector members;
} Group;

void compute_section_sizes(Context *ctx) {
    for(int i = 0;i<ctx->chunks.size;i++) {
        OutputSection *osec = ((Chunk *)ctx->chunks.data[i])->outsec;
        if (!osec)
            continue;
        if (target.thunk_size) {
            if ((*osec->chunk->shdr.sh_flags.val & SHF_EXECINSTR) || !ctx->arg.relocatable) {
                continue;
            }
        }
        vector groups;
        i64 group_size = 10000;
        VectorNew(&groups,1);
        vector split_result = split(osec->member, group_size);
        for (int j = 0;j < split_result.size;j++) {
            vector *span = (vector *)split_result.data[i];
            Group group = {.members = *span};
            VectorAdd(&groups,&group,sizeof(Group *));
        }

        for(int i = 0;i < groups.size;i++) {
            Group *group = (Group *)groups.data[i];
            for(int j = 0;;j++) {
                InputSection *isec = (InputSection *)group->members.data[i];
                group->size = align_to(group->size, 1 << isec->p2align) + isec->sh_size;
                group->p2align = group->p2align > isec->p2align ? group->p2align : isec->p2align;
            }           
        }

        ElfShdr *shdr = &(osec->chunk->shdr);
        *shdr->sh_size.val = 0;

        for (i64 i = 0; i < groups.size; i++) {
            Group *group = (Group *)groups.data[i];
            *shdr->sh_size.val = align_to(*shdr->sh_size.val, 1 << group->p2align);
            group->offset = *shdr->sh_size.val;
            *shdr->sh_size.val += group->size;
            *shdr->sh_addralign.val = *shdr->sh_addralign.val > 1 << group->p2align ? *shdr->sh_addralign.val : 1 << group->p2align;
        }

        // Assign offsets to input sections.
        for(int i = 0;i < groups.size;i++) {
            Group *group = (Group *)groups.data[i];
            i64 offset = group->offset;
            for(int j = 0;j < group->members.size;j++) {
                InputSection *isec = group->members.data[i];
                offset = align_to(offset,1 << isec->p2align);
                isec->offset = offset;
                offset += isec->sh_size;
            }
        }
    }

    // 对ARM处理
    if(target.thunk_size) {
        if (!ctx->arg.relocatable) {
            for (int i = 0;i < ctx->chunks.size;i++) {
                Chunk *chunk = ctx->chunks.data[i];
                OutputSection *osec = chunk->outsec;
                
                if (!osec)
                    continue;
                if (*osec->chunk->shdr.sh_flags.val & SHF_EXECINSTR)
                    create_range_extension_thunks(ctx,osec);
            }
        }
    }
}

i64 get_rank1(Context *ctx,Chunk *chunk) {
    u64 type = *chunk->shdr.sh_type.val;
    u64 flags = *chunk->shdr.sh_flags.val;
    if (chunk == ctx->ehdr->chunk)
        return 0;
    if (chunk == ctx->phdr->chunk)
        return 1;
    if (type == SHT_NOTE && (flags & SHF_ALLOC))
        return 3;
    if (chunk == ctx->hash->chunk)
        return 4;
    if (chunk == ctx->gnu_hash->chunk)
        return 5;
    if (chunk == ctx->dynsym->chunk)
        return 6;
    if (chunk == ctx->dynstr->chunk)
        return 7;
    if (chunk == ctx->versym->chunk)
        return 8;
    if (chunk == ctx->verneed->chunk)
        return 9;
    if (chunk == ctx->reldyn->chunk)
        return 10;
    if (chunk == ctx->relplt->chunk)
        return 11;
    if (chunk == ctx->shdr->chunk)
        return INT32_MAX - 1;

    bool alloc = (flags & SHF_ALLOC);
    bool writable = (flags & SHF_WRITE);
    bool exec = (flags & SHF_EXECINSTR);
    bool tls = (flags & SHF_TLS);
    bool relro = chunk->is_relro;
    bool is_bss = (type == SHT_NOBITS);

    return (1 << 10) | (!alloc << 9) | (writable << 8) | (exec << 7) |
           (!tls << 6) | (!relro << 5) | (is_bss << 4);
}

i64 get_rank2(Context *ctx,Chunk *chunk) {
    ElfShdr *shdr = &(chunk->shdr);
    if (*shdr->sh_type.val == SHT_NOTE)
        return -(*shdr->sh_addralign.val);
    if (chunk == ctx->got->chunk)
        return 2;
    if (chunk->name && !strcmp(chunk->name,".toc"))
        return 3;
    if (chunk->name && !strcmp(chunk->name,".alpha_got"))
        return 4;

    if (*shdr->sh_flags.val & SHF_MERGE) {
        if (*shdr->sh_flags.val & SHF_STRINGS) 
            return (5LL << 32) | *shdr->sh_entsize.val;
        return (6LL << 32) | *shdr->sh_entsize.val;
    }

    if (chunk == ctx->relro_padding->chunk)
        return INT64_MAX;
    return 0;
}

int compare_chunks_out(const void* a, const void* b) {
    Chunk* chunk_a = *(Chunk**)a;
    Chunk* chunk_b = *(Chunk**)b;
    
    u64 rank1_a = get_rank1(global_ctx,chunk_a);
    u64 rank1_b = get_rank1(global_ctx,chunk_b);
    if (rank1_a < rank1_b) 
        return -1;
    else if (rank1_a > rank1_b)
        return 1;    
    
    u64 rank2_a = get_rank2(global_ctx,chunk_a);
    u64 rank2_b = get_rank2(global_ctx,chunk_b);
    if (rank2_a < rank2_b) 
        return -1;
    else if (rank2_a > rank2_b)
        return 1;
    
    if (strcmp((chunk_a)->name, (chunk_b)->name) < 0) 
        return -1;
    else if(strcmp((chunk_a)->name, (chunk_b)->name) > 0)
        return 1;
    return 0;
}

void sort_output_sections_regular(Context *ctx) {
    global_ctx = ctx;
    size_t size = ctx->chunks.size;
    // Chunk *temp1[ctx->chunks.size];
    // for (int i = 0; i < ctx->chunks.size; i++) {
    //     temp1[i] = (Chunk *)(ctx->chunks.data[i]);
    // }
    // qsort(temp1, ctx->chunks.size, sizeof(Chunk *), compare_chunks_out);
    qsort(ctx->chunks.data, ctx->chunks.size, sizeof(Chunk *), compare_chunks_out);
    for (int i = 0; i < size; i++) {
        // 获取当前元素
        Chunk* currentElement = ctx->chunks.data[i];
        printf("finalname: %s\n", currentElement->name);
    } 
    // ctx->chunks.data = NULL;
    // VectorNew(&(ctx->chunks),1);
    // // 验证
    // for (int i = 0; i < size; i++) {
    //     // 获取当前元素
    //     Chunk* currentElement = temp1[i];
    //     printf("finalname: %s\n", currentElement->name);
    //     VectorAdd(&(ctx->chunks),currentElement,sizeof(Chunk *));
    // } 
}

void sort_output_sections(Context *ctx) {
    if (ctx->arg.section_order.size == 0)
        sort_output_sections_regular(ctx);
}

void create_output_symtab(Context *ctx) {
    if (!ctx->arg.strip_all && !ctx->arg.retain_symbols_file.size) {
        for(int i = 0;i < ctx->chunks.size;i++) {
            Chunk *chunk = ctx->chunks.data[i];
            if (chunk->is_outsec)
                output_sec_compute_symtab_size(ctx,chunk);
            if (chunk->is_gotsec)
                got_sec_compute_symtab_size(ctx,chunk);
            if (chunk->is_plt)
                plt_sec_compute_symtab_size(ctx,chunk);
            if (chunk->is_pltgot)
                pltgot_sec_compute_symtab_size(ctx,chunk);
        }
    }
    for(int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = ctx->objs.data[i];
        obj_compute_symtab_size(ctx,file);
    }
}

void update_shdr(Context *ctx) {
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if (!strcmp(chunk->name,"PHDR")) 
            output_phdr_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".rel.dyn"))
            reldyn_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".strtab"))
            strtab_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".shstrtab"))
            shstrtab_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".symtab"))
            symtab_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".got.plt"))
            gotplt_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".plt"))
            plt_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".rel.plt"))
            relplt_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".dynsym"))
            dynsym_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".hash"))
            hash_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".gnu.hash"))
            gnu_hash_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".eh_frame_hdr"))
            eh_frame_hdr_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".gnu.version"))
            gnu_version_update_shdr(ctx,chunk);
        if (!strcmp(chunk->name,".gnu.version_r"))
            gnu_version_r_update_shdr(ctx,chunk);
    }
}
void compute_section_headers(Context *ctx) {
    int p = 0;
    // 所有输出段更新shdr
    update_shdr(ctx);

    // Remove empty chunks.
    for (int i = 0; i < ctx->chunks.size; ) {
        Chunk *chunk = ctx->chunks.data[i];
        if (kind(chunk) != OUTPUT_SECTION  &&
            *chunk->shdr.sh_size.val == 0) {
            // 删除当前元素
            free(chunk);
            for (int j = i; j < ctx->chunks.size - 1; j++) {
                ctx->chunks.data[j] = ctx->chunks.data[j+1];
            }
            ctx->chunks.size--;
        } else {
            i++;
        }
    }

    // Set section indices.
    i64 shndx = 1;
    for(i64 i = 0;i < ctx->chunks.size;i++) 
        if(kind(ctx->chunks.data[i]) != HEADER)
            ((Chunk *)ctx->chunks.data[i])->shndx = shndx++;
    
    if(ctx->shdr)
        *ctx->shdr->chunk->shdr.sh_size.val = shndx * sizeof(ElfShdr);

    update_shdr(ctx);  
}

i64 get_flags(Context *ctx,Chunk *chunk) {
    i64 RELRO = 1LL << 32;
    i64 flags = to_phdr_flags(ctx, chunk);
    if(chunk->is_relro)
        return flags | RELRO;
    return flags;
}

void set_virtual_addresses_regular (Context *ctx) {
    i64 RELRO = 1LL << 32;
    vector chunks = ctx->chunks;
    u64 addr = ctx->arg.image_base;

    // 对tls chunk计算一个align
    Chunk *first_tls_chunk = NULL;
    u64 tls_alignment = 1;
    for(int i = 0;i < chunks.size;i++) {
        Chunk *chunk = chunks.data[i];
        if(*chunk->shdr.sh_flags.val & SHF_TLS) {
            if(!first_tls_chunk)
                first_tls_chunk = chunk;
            tls_alignment = tls_alignment > (u64)(*chunk->shdr.sh_addralign.val) ? tls_alignment : (u64)(*chunk->shdr.sh_addralign.val);
        }
    }

    for(i64 i = 0;i < chunks.size;i++) {
        if(!(*((Chunk *)chunks.data[i])->shdr.sh_flags.val & SHF_ALLOC))
            continue;

        if(chunks.data[i] == ctx->relro_padding) {
            *((Chunk *)chunks.data[i])->shdr.sh_addr.val = addr;
            *((Chunk *)chunks.data[i])->shdr.sh_size.val = align_to(addr,ctx->page_size);
            addr += ctx->page_size;
            continue;
        }

        if (i > 0 && chunks.data[i - 1] != ctx->relro_padding) {
            i64 flags1 = get_flags(ctx,chunks.data[i - 1]);
            i64 flags2 = get_flags(ctx,chunks.data[i]);

            if (!ctx->arg.nmagic && flags1 != flags2) {
                switch (ctx->arg.z_separate_code) {
                    case SEPARATE_LOADABLE_SEGMENTS:
                        addr = align_to(addr, ctx->page_size);
                        break;
                    case SEPARATE_CODE:
                        if ((flags1 & PF_X) != (flags2 & PF_X)) {
                            addr = align_to(addr, ctx->page_size);
                            break;
                        }
                    case NOSEPARATE_CODE:
                        if (addr % ctx->page_size != 0)
                            addr += ctx->page_size;
                        break;
                    default:
                        assert(0 && "unreachable");
                }
            }
        }

        u64 res = chunks.data[i] == first_tls_chunk ? tls_alignment : (u64)(*((Chunk *)chunks.data[i])->shdr.sh_addralign.val);
        addr = align_to(addr, res);
        *((Chunk *)chunks.data[i])->shdr.sh_addr.val = addr;
        addr += *((Chunk *)chunks.data[i])->shdr.sh_size.val;
    }
}

static u64 align_with_skew(u64 val, u64 align, u64 skew) {
    u64 x = align_down(val, align) + skew % align;
    return (val <= x) ? x : x + align;
}

i64 set_file_offsets(Context *ctx) {
    vector chunks = ctx->chunks;
    u64 fileoff = 0;
    i64 i = 0;
    while(i < chunks.size) {
        Chunk *first = chunks.data[i];

        if(!(*first->shdr.sh_flags.val & SHF_ALLOC)) {
            fileoff = align_to(fileoff,*first->shdr.sh_addralign.val);
            *first->shdr.sh_offset.val = fileoff;
            fileoff += *first->shdr.sh_size.val;
            i++;
            continue;
        }
        if (*first->shdr.sh_type.val == SHT_NOBITS) {
            i++;
            continue;
        }

        if(*first->shdr.sh_addralign.val > ctx->page_size)
            fileoff = align_to(fileoff,*first->shdr.sh_addralign.val);
        else
            fileoff = align_with_skew(fileoff,ctx->page_size,*first->shdr.sh_addr.val);
        
        // 分配ALLOC段连续的文件偏移量 在内存中是连续的
        for(;;) {
            *((Chunk *)chunks.data[i])->shdr.sh_offset.val = fileoff + *((Chunk *)chunks.data[i])->shdr.sh_addr.val - *first->shdr.sh_addr.val;
            i++;

            if(i >= chunks.size ||
                !(*((Chunk *)chunks.data[i])->shdr.sh_flags.val & SHF_ALLOC) ||
                *((Chunk *)chunks.data[i])->shdr.sh_type.val == SHT_NOBITS)
                break;
            
            if (*((Chunk *)chunks.data[i])->shdr.sh_addr.val < *first->shdr.sh_addr.val)
                break;
            
            i64 gap_size = *((Chunk *)chunks.data[i])->shdr.sh_addr.val - *((Chunk *)chunks.data[i - 1])->shdr.sh_addr.val -
                            *((Chunk *)chunks.data[i - 1])->shdr.sh_size.val;
            
            if (gap_size >= ctx->page_size)
                break;
        }

        fileoff = *((Chunk *)chunks.data[i - 1])->shdr.sh_offset.val + *((Chunk *)chunks.data[i - 1])->shdr.sh_size.val;

        while(i < chunks.size &&
            (*((Chunk *)chunks.data[i])->shdr.sh_flags.val &  SHF_ALLOC) &&
            *((Chunk *)chunks.data[i])->shdr.sh_type.val == SHT_NOBITS)
            i++;
    } 
    return fileoff;
}

i64 set_osec_offsets(Context *ctx) {
    for (;;) {
        // 设置段的虚拟地址
        if (ctx->arg.section_order.size == 0)
            set_virtual_addresses_regular(ctx);

        // 设置段在文件中的offset
        i64 fileoff = set_file_offsets(ctx);

        if (ctx->phdr) {
            i64 sz = *ctx->phdr->chunk->shdr.sh_size.val;
            output_phdr_update_shdr(ctx,ctx->phdr->chunk);
            if (sz != *ctx->phdr->chunk->shdr.sh_size.val)
                continue;
        }

        // 返回文件的长度
        return fileoff;
    }
}

void start(ELFSymbol *sym, Chunk *chunk) {
    i64 bias = 0;
    if (sym && chunk) {
        set_output_section(chunk,sym);
        sym->value = *chunk->shdr.sh_addr.val + bias;
    }
}

Chunk *find(char *name,vector sections) {
    for(int i = 0;i < sections.size;i++) {
        Chunk *chunk = sections.data[i];
        if (!strcmp(chunk->name,name))
            return chunk;
    }
    return NULL;
}

void stop(ELFSymbol *sym, Chunk *chunk) {
    if (sym && chunk) {
        set_output_section(chunk,sym);
        sym->value = *chunk->shdr.sh_addr.val + *chunk->shdr.sh_size.val;
    }
}

void fix_synthetic_symbols(Context *ctx) {
    vector sections;
    VectorNew(&sections,1);
    for(int i = 0;i < ctx->chunks.size;i++) {
        Chunk *chunk = ctx->chunks.data[i];
        if(kind(chunk) != HEADER && (*chunk->shdr.sh_flags.val & SHF_ALLOC)) 
            VectorAdd(&sections,chunk,sizeof(Chunk *));
    }

    // __bss_start
    Chunk *chunk = find(".bss",sections);
    if (chunk)
        start(ctx->__bss_start, chunk);

    if (ctx->ehdr && (*ctx->ehdr->chunk->shdr.sh_flags.val & SHF_ALLOC)) {
        set_output_section(sections.data[0],ctx->__ehdr_start);
        ctx->__ehdr_start->value = *ctx->ehdr->chunk->shdr.sh_addr.val;
        set_output_section(sections.data[0],ctx->__executable_start);
        ctx->__executable_start->value = *ctx->ehdr->chunk->shdr.sh_addr.val;
    }

    if (ctx->__dso_handle) {
        set_output_section(sections.data[0],ctx->__dso_handle);
        ctx->__dso_handle->value = *((Chunk *)sections.data[0])->shdr.sh_addr.val;
    }

    // _end, _etext, _edata and the like
    for(int i = 0;i < sections.size;i++) {
        Chunk *chunk = sections.data[i];
        if (*chunk->shdr.sh_flags.val & SHF_ALLOC) {
            stop(ctx->_end, chunk);
            stop(ctx->end, chunk);
        }

        if (*chunk->shdr.sh_flags.val & SHF_EXECINSTR) {
            stop(ctx->_etext, chunk);
            stop(ctx->etext, chunk);
        }

        if(*chunk->shdr.sh_type.val != SHT_NOBITS &&
            (*chunk->shdr.sh_flags.val & SHF_ALLOC)) {
            stop(ctx->_edata, chunk);
            stop(ctx->edata, chunk);
        }
    }

    // _DYNAMIC
    start(ctx->_DYNAMIC, ctx->dynamic->chunk);

    start(ctx->_GLOBAL_OFFSET_TABLE_, ctx->got->chunk);

    // _PROCEDURE_LINKAGE_TABLE_. We need this on SPARC.
    start(ctx->_PROCEDURE_LINKAGE_TABLE_, ctx->plt->chunk);

    if (ctx->_TLS_MODULE_BASE_) {
        set_output_section(sections.data[0],ctx->_TLS_MODULE_BASE_);
        ctx->_TLS_MODULE_BASE_->value = ctx->dtp_addr;
    }

    // __GNU_EH_FRAME_HDR
    start(ctx->__GNU_EH_FRAME_HDR, ctx->eh_frame_hdr->chunk);

    // ARM32's __exidx_{start,end}
    if (ctx->__exidx_start) {
        Chunk *chunk = find(".ARM.exidx",sections);
        if (chunk) {
            start(ctx->__exidx_start, chunk);
            stop(ctx->__exidx_end, chunk);
        }
    }

    // __start_ and __stop_ symbols
    // 貌似没有

}

void copy_chunks(Context *ctx) {
    
}

