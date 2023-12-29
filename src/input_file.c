
#include "mold.h"
#define Fatal(ctx) printf("%s: ", ctx)
bool is_undef(ElfSym *sym) { return *(u16 *)&(sym->st_shndx) == SHN_UNDEF; }
bool is_abs(ElfSym *sym) { return *(u16 *)&(sym->st_shndx) == SHN_ABS; }
bool is_common(ElfSym *sym) { return *(u16 *)&(sym->st_shndx) == SHN_COMMON; }
bool is_weak(ElfSym *sym) { return sym->st_bind == STB_WEAK; }
bool is_undef_weak(ElfSym *sym) { return is_undef(sym) && is_weak(sym); }

static const size_t npos = (size_t)-1;
int num_sections;
StringView get_data(Context *ctx,ElfShdr *shdr) {
    StringView view = get_string(ctx, shdr);
    return view;
}

ElfShdr *find_section (i64 type) {
    for (i64 i = 0; i < 10; i++) {
        
        // elf_sections[i] = sh_begin + i;
        ElfShdr *sec = elf_sections[i];
        if (*sec->sh_type.val == type)  {
            return sec;
        }
    }
    return NULL;
}

InputSection* create_input_section(/* 构造参数 */) {
    InputSection* section = malloc(sizeof(InputSection));
    if (section == NULL) {
        /* 处理内存分配失败的情况 */
        return NULL;
    }
    section->fde_begin = -1;
    section->fde_end = -1;
    section->offset = -1;
    section->shndx = -1;
    section->relsec_idx = -1;
    section->reldyn_offset = 0;
    section->sh_size = -1;
    section->uncompressed = false;
    section->is_visited = false;
    section->p2align = 0;
    section->is_alive = true;
    return section;
}

/* 销毁 InputSection 对象的函数 */
void destroy_input_section(InputSection* section) {
    /* 释放 InputSection 对象占用的资源 */
    /* 删除已经分配的内存等 */
    free(section);
}

/* 销毁 InputSection 对象数组 */
void destroy_input_sections(InputSection** sections, size_t num_sections) {
    if (sections == NULL) {
        return;
    }
    for (size_t i = 0; i < num_sections; ++i) {
        destroy_input_section(sections[i]);
    }
    free(sections);
}

InputSection** create_input_sections(size_t num_sections) {
    InputSection** sections = malloc(num_sections * sizeof(InputSection*));
    if (sections == NULL) {
        /* 处理内存分配失败的情况 */
        return NULL;
    }
    for (size_t i = 0; i < num_sections; ++i) {
        sections[i] = create_input_section(/* 构造参数 */);
        if (sections[i] == NULL) {
            /* 处理对象创建失败的情况 */
            for (size_t j = 0; j < i; ++j) {
                destroy_input_section(sections[j]); /* 销毁已经创建的对象 */
            }
            free(sections); /* 释放已经分配的内存 */
            return NULL;
        }
    }
    return sections;
}

/* 添加新的 InputSection 对象到已有的数组中 */
InputSection** add_input_section(InputSection** sections, size_t num_sections) {
    InputSection** new_sections = realloc(sections, (num_sections + 1) * sizeof(InputSection*));
    if (new_sections == NULL) {
        /* 处理内存分配失败的情况 */
        return sections;
    }
    sections = new_sections;
    sections[num_sections] = create_input_section(/* 构造参数 */);
    if (sections[num_sections] == NULL) {
        /* 处理对象创建失败的情况 */
        return sections;
    }
    return sections;
}

void assignStringView(StringView* view, ElfSym*** elf_syms) {
    *elf_syms = (ElfSym**)malloc(sizeof(ElfSym*));
    if (*elf_syms != NULL) {
        **elf_syms = (ElfSym*)malloc(view->size);
        if (**elf_syms != NULL) {
            memcpy(**elf_syms, view->data, view->size);
        }
        else {
            // 内存分配失败，进行错误处理
            // ...
            free(*elf_syms);
            *elf_syms = NULL;
        }
    }
}

void validateElfSyms(ElfSym** elf_syms, size_t num_elements) {
    for (size_t i = 0; i < num_elements; ++i) {
        ElfSym* temp = elf_syms[i];
        if (temp == NULL) {
            return;
        }
        // 打印每个元素的值，或进行其他验证操作
        printf("elf_syms[%zu]: %d\n", i, *elf_syms[i]->st_shndx.val);
    }
}

void *inputfile_copy(Context *ctx, MappedFile *mf,
                          char *archive_name, bool is_in_lib,ObjectFile *obj) {
    if (mf->size < sizeof(ElfEhdr)) 
        fprintf(stderr, "file too small: %s\n",strerror(errno));     
    if (memcmp(mf->data, "\177ELF", 4))
        fprintf(stderr, "not an ELF file: %s\n",strerror(errno));  

    inputfile_mf = mf;
    obj->inputfile.inputfile_mf = mf;
    //创建ELF header
    ElfEhdr *ehdr = (ElfEhdr *)mf->data;
    Inputefile input;
    input.is_dso = (*ehdr->e_type.val == ET_DYN);        
    size_t e_shoff_size = sizeof(ehdr->e_shoff);
    // e_shoff是节头表文件偏移量   所以sh_begin应该是一个ELF header的末尾
    uint16_t value = (ehdr->e_shoff.val[1] << 8) | ehdr->e_shoff.val[0];
    // 将 value 格式化为十六进制字节
    // printf("Value in hexadecimal: 0x%04X\n", value);
    ElfShdr *sh_begin = (ElfShdr *)((unsigned char*)mf->data + value);
    // e_shnum是节的数量
    i64 num_sections = (*ehdr->e_shnum.val == 0) ? *sh_begin->sh_size.val : *ehdr->e_shnum.val; 

    if (mf->data + mf->size < (u8 *)(sh_begin + num_sections))
        printf("%s: e_shoff or e_shnum corrupted: %lld %ld\n", mf->name, mf->size, num_sections);
    elf_sections = (ElfShdr **)malloc(sizeof(ElfShdr *) * num_sections);
    for (i64 i = 0; i < num_sections; i++) {
        elf_sections[i] = sh_begin + i;
        ElfShdr *temp = elf_sections[i];
    }
    obj->inputfile.elf_sections = (ElfShdr **)malloc(sizeof(ElfShdr *) * num_sections);
    obj->inputfile.elf_sections = elf_sections;
    obj->inputfile.elf_sections_num = num_sections;
    i64 shstrtab_idx = (*ehdr->e_shstrndx.val == SHN_XINDEX)
                ? *sh_begin->sh_link.val : *ehdr->e_shstrndx.val;
    shstrtab = get_string(ctx, elf_sections[shstrtab_idx]);
    obj->inputfile.shstrtab = shstrtab;
}

ObjectFile * objectfile_create(Context *ctx,MappedFile *mf,char *archive_name, bool is_in_lib) {
    ObjectFile* obj = malloc(sizeof(ObjectFile));
    inputfile_copy(ctx, mf, archive_name, is_in_lib,obj);
    
    obj->archive_name = strdup(archive_name);
    obj->exclude_libs = false;
    obj->is_in_lib = false;
    obj->is_lto_obj = false;
    obj->needs_executable_stack = false;
    obj->num_dynrel = 0;
    obj->reldyn_offset = 0;
    obj->fde_idx = 0;
    obj->fde_offset = 0;
    obj->fde_size = 0;
    obj->has_common_symbol = false;
    return obj;
}

void initialize_sections(Context *ctx,ObjectFile *file) {
    num_sections = 0;
    ElfShdr** section_ptr = elf_sections;

    while (*section_ptr != NULL) {
        num_sections++;
        section_ptr++;
    }

    file->sections = create_input_sections(num_sections);
    file->num_sections = num_sections;
    for (size_t i = 0; i < file->inputfile.elf_sections_num; ++i) {
        ElfShdr* shdr = elf_sections[i];
        file->sections[i]->is_alive = true;
        file->sections[i]->is_constucted = false;
        if ((*shdr->sh_flags.val & SHF_EXCLUDE) && !(*shdr->sh_flags.val & SHF_ALLOC) &&
            *shdr->sh_type.val != SHT_LLVM_ADDRSIG && !ctx->arg.relocatable)
            continue;
        // 在此处执行对每个 section 的操作
        switch (*shdr->sh_type.val) {
            case SHT_REL:
            case SHT_RELA:
            case SHT_SYMTAB:
            case SHT_SYMTAB_SHNDX:
            case SHT_STRTAB:
            case SHT_NULL:
                break;
            default: {
                uint16_t value = *(u32 *)&(shdr->sh_name);
                char *name = (char *)(shstrtab.data + value);
                if (!strcmp(name,".note.GNU-stack") && !ctx->arg.relocatable) {
                    if (*shdr->sh_flags.val & SHF_EXECINSTR) {
                    }
                    continue;
                }

                file->sections[i]->contents = (StringView *)malloc(sizeof(StringView));
                file->sections[i]->is_constucted = true;
                define_input_section(file,ctx,i);              
                break;
            }
        }
    }

    sections = file->sections;
    for (size_t i = 0; i < file->inputfile.elf_sections_num; ++i) {
        ElfShdr* shdr = elf_sections[i];
        if (*shdr->sh_type.val != (target.is_rela ? SHT_RELA : SHT_REL))
            continue;
    }
}

void initialize_symbols (Context *ctx,ObjectFile *obj) {
    if (!obj->symtab_sec)
        return;
    VectorNew(&(obj->symbols),1);
    // Initialize local symbols
    VectorNew(&local_syms,10);
    ELFSymbol *sym = (ELFSymbol *)malloc(sizeof(ELFSymbol));
    sym_init(sym);
    sym->file = obj;
    sym->sym_idx = 0;
    // Vectorpush_back(&local_syms,&sym);
    VectorAdd(&local_syms,sym,sizeof(ELFSymbol *));
    // 70c8 0x7ffff7fc70b8 0x7ffff7fc70a8
    i64 i = 0;
    ELFSymbol *start_sym = (ELFSymbol *)malloc(sizeof(ELFSymbol));
    sym_init(start_sym);
    VectorAdd(&symbols,start_sym,sizeof(ElfSym *));
    ElfSym *esym0 = (ElfSym *)(elf_syms + 0);
    VectorAdd(&(obj->inputfile.elf_syms),esym0,sizeof(ElfSym *));
    for (i = 2; i < first_global * 2; i = i + 2) {
        ElfSym *esym = (ElfSym *)(elf_syms + i);
        char *name;
        uint32_t value = (esym->st_name.val[3] << 24) | (esym->st_name.val[2] << 16) | (esym->st_name.val[1] << 8) | esym->st_name.val[0];
        // name = symbol_strtab.data + value;
        if (esym->st_type == STT_SECTION) {
            i64 shndx = get_shndx(esym);
            name = (char *)(shstrtab.data + *(u32 *)&(elf_sections[shndx]->sh_name));
        } else {
            name = (char *)(symbol_strtab.data + *(u32 *)&(esym->st_name));
        }
        ELFSymbol *sym2 = (ELFSymbol *)malloc(sizeof(ELFSymbol));
        sym_init(sym2);
        sym2->file = obj;
        sym2->nameptr = strdup(name);
        sym2->namelen = strlen(name);
        sym2->value = *(u32 *)&(esym->st_value);
        sym2->sym_idx = (i + 1) / 2;
        VectorAdd(&local_syms,sym2,sizeof(ELFSymbol *));
        VectorAdd(&(obj->inputfile.elf_syms),esym,sizeof(ElfSym *));

        if (!is_abs(esym))
            set_input_section(obj->sections[get_shndx(esym)],sym2);
        // Vectorpush_back(&local_syms,&sym2);
    }
    VectorNew(&(obj->inputfile.local_syms),10);
    obj->inputfile.local_syms = local_syms;
    i64 num_elf_sym = i / 2 + 1;
    VectorNew(&symbols,num_elf_sym);
    VectorNew(&(obj->inputfile.symbols),num_elf_sym);
    
    i64 num_globals = num_elf_sym - first_global;
    resize(&has_symver,num_globals);

    for (i64 i = 0; i < first_global; i++) {
        ELFSymbol *tt1 = local_syms.data[i];
        VectorAdd(&symbols,tt1,sizeof(ELFSymbol));
    }
    
    // Initialize global symbols
    for (i64 i = first_global; i < num_elf_sym; i++) {
        ElfSym *esym = (ElfSym *)(elf_syms + i * 2);
        char *key = (char *)(symbol_strtab.data + *esym->st_name.val);
        char *name = key;
        VectorAdd(&(obj->inputfile.elf_syms),esym,sizeof(ElfSym *));
        
        ELFSymbol *symbol = insert_symbol(ctx,key,name);
        sym_init(symbol);
        symbol->file = NULL;
        symbol->sym_idx = i;  
        VectorAdd(&symbols,symbol,sizeof(ELFSymbol *));
    }
    obj->inputfile.symbols = symbols;
    obj->symbols = symbols;
    
}

void parse(Context *ctx,ObjectFile *obj) {
    obj->symtab_sec = find_section(SHT_SYMTAB);

    if (obj->symtab_sec) {
        first_global = *obj->symtab_sec->sh_info.val;
        obj->inputfile.first_global = first_global;
        StringView view = get_data(ctx, obj->symtab_sec);
        // ElfSym* elf_syms;
        // assignStringView(&view, &elf_syms); 
        elf_syms = (ElfSym **)view.data;
        size_t size = view.size / sizeof(ElfSym);
        VectorNew(&(obj->inputfile.elf_syms),size);
        
        // obj->inputfile.elf_syms = (ElfSym **)view.data; 
        // validateElfSyms(elf_syms, view.size / sizeof(ElfSym));
        // 0x7ffff7fc70b8 0x7ffff7fc70a8
        i64 idx = *(obj->symtab_sec)->sh_link.val;
        symbol_strtab = get_string(ctx,elf_sections[idx]);
        obj->inputfile.symbol_strtab = symbol_strtab;
    }
    VectorNew(&symbols,1);

    initialize_sections(ctx,obj);
    initialize_symbols(ctx,obj);
    // sort_relocations(ctx);
    // parse_ehframe(ctx);
}

static u64 get_sym_rank(ElfSym *esym, Inputefile *file, bool is_in_archive) {
    if (is_common(esym)) {
        assert(!file->is_dso);
        return is_in_archive ? 6 : 5;
    }

    if (file->is_dso || is_in_archive)
        return (esym->st_bind == STB_WEAK) ? 4 : 3;

    if (esym->st_bind == STB_WEAK)
        return 2;
    return 1;
}

static u64 get_rank (Inputefile *file,ElfSym *esym, bool is_in_archive) {
    return (get_sym_rank(esym,file,is_in_archive) << 24) + file->priority;
}

static u64 get_rank_single_sym(ELFSymbol *sym,i64 i) {
    if(sym) {
        if (!sym->file)
            return 7 << 24;
        return get_rank(&(sym->file->inputfile), esym(&(sym->file->inputfile),i), !sym->file->inputfile.is_alive);
    }
}

// 定义函数，用于创建一个智能指针，并初始化引用计数为 1
SmartPointer *create_smart_pointer(ObjectFile *obj) {
    SmartPointer *sp = (SmartPointer *)malloc(sizeof(SmartPointer));
    sp->ptr = obj;
    sp->ref_count = 1;
    return sp;
}
// 定义函数，用于增加智能指针的引用计数
void increase_ref_count(SmartPointer *sp) {
    sp->ref_count++;
}
// 定义函数，类似于 C++ 中的 emplace_back() 函数，用于向对象池中添加一个对象
void add_object_to_pool(ObjectFile **pool, int *size, ObjectFile *obj) {
    // 创建一个智能指针，将引用计数初始化为 1
    SmartPointer *sp = create_smart_pointer(obj);

    // 将新的智能指针加入到数组中
    pool[*size] = obj;
    (*size)++;

    // 增加智能指针的引用计数
    increase_ref_count(sp);
}

size_t find_null(const char* data, size_t data_size, u64 entsize) {
    if (entsize == 1) {
        for (size_t i = 0; i < data_size; i++) {
            if (data[i] == '\0') {
                return i;
            }
        }
        return data_size; // 返回 data_size 表示未找到 null 字符
    }

    for (size_t i = 0; i <= data_size - entsize; i += entsize) {
        int found = 1;
        for (size_t j = 0; j < entsize; j++) {
            if (data[i + j] != '\0') {
                found = 0;
                break;
            }
        }
        if (found) {
            return i;
        }
    }

    return data_size; // 返回 data_size 表示未找到 null 字符
}

StringView *create_string_view(const char* data, size_t size) {
    StringView *view = (StringView *)malloc(sizeof(StringView));
    view->data = data;
    view->size = size;
    return view;
}
// 截取子串函数
StringView *substring(StringView *view, size_t start, size_t length) {
    const char* data = view->data + start;
    size_t size = (length <= view->size - start) ? length : (view->size - start);
    return create_string_view(data, size);
}

MergeableSection *split_section(Context *ctx,InputSection *sec,ObjectFile *file,int i) {
    if (!sec->is_alive || sec->relsec_idx != -1) 
        return NULL;
    ElfShdr *shdr = get_shdr(file,i);
    if (!(*shdr->sh_flags.val & SHF_MERGE))
        return NULL;
    i64 entsize = *shdr->sh_entsize.val;
    if (entsize == 0)
        entsize = (*shdr->sh_flags.val & SHF_STRINGS) ? 1 : (int)(*shdr->sh_addralign.val);

    if (entsize == 0)
        return NULL;
    i64 addralign = *shdr->sh_addralign.val;
    if (addralign == 0)
        addralign = 1;
    MergeableSection *rec = (MergeableSection *)malloc(sizeof(MergeableSection));
    rec->parent = get_instance(ctx,get_name(file,i),*shdr->sh_type.val,*shdr->sh_flags.val, entsize, addralign);
    rec->p2align = sec->p2align;
    if (sec->sh_size == 0) 
        return sec;
    input_sec_uncompress(ctx,file,sec,i);

    StringView *data = sec->contents;
    const char *begin = data->data;
    if (*shdr->sh_flags.val & SHF_STRINGS) {
        VectorNew(&rec->strings,1);
        VectorNew(&rec->frag_offsets,1);
        VectorNew(&rec->hashes,1);
        while(data->size != 0) {
            size_t end = find_null(data->data,data->size,entsize);
            StringView *substr = substring(data, 0, end + entsize);
            data = substring(data,end + entsize, data->size - (end + entsize));

            VectorAdd(&rec->strings,substr,sizeof(char *));
            
            VectorAdd(&rec->frag_offsets,(u32 *)(substr->data - begin),sizeof(u32));
            u64 hash = hash_string((char *)substr->data);
            VectorAdd(&rec->hashes,&hash,sizeof(u64));
        }
    } else {
        if (data->size % entsize)
            Fatal(": section size is not multiple of sh_entsize");
        while(data->size != 0) {
            StringView *substr = substring(data,0,entsize);
            data = substring(data,entsize, data->size - (entsize));
            VectorNew(&rec->strings,1);
            VectorNew(&rec->frag_offsets,1);
            VectorNew(&rec->hashes,1);

            VectorAdd(&rec->strings,&substr,sizeof(char *));
            
            VectorAdd(&rec->frag_offsets,(u32 *)(substr->data - begin),sizeof(u32));
            u64 hash = hash_string((char *)substr->data);
            VectorAdd(&rec->hashes,&hash,sizeof(u64));
        }
    }
    return rec;
}

void initialize_mergeable_sections(Context *ctx,ObjectFile *file) {
    VectorNew(&(file->mergeable_sections),file->inputfile.elf_sections_num);

    for (i64 i = 1; i < file->inputfile.elf_sections_num; i++) {
        if (file->sections == NULL)
            break;
        InputSection *isec = file->sections[i];
        if (isec) {
            MergeableSection *m = split_section(ctx, isec,file,i);
            
            if (m) {
                VectorAdd(&(file->mergeable_sections),m,sizeof(MergeableSection *));
                isec->is_alive = false;
            }
            
        }
    }
}

int UpperBound(i64 *arr, int size, i64 value) {
    int left = 0, right = size;
    while (left < right) {
        int mid = (left + right) >> 1;
        if (arr[mid] > value) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    return left;
}

SectionFragment *get_fragment(i64 offset, MergeableSection *m) {
    vector vec;
    VectorNew(&vec,1);
    vec = m->frag_offsets;

    i64 *vec_data = (i64 *)vec.data;
    int vec_size = vec.size;
    int idx = UpperBound(vec_data, vec_size, offset) - 1;
    if (idx >= 0) {
        SectionFragment *result = (SectionFragment *)m->fragments.data[idx];
        return result;
    } else {
        SectionFragment *result = NULL;
        return result;
    }
}

void file_resolve_section_pieces(Context *ctx,ObjectFile *file) {
    for(int i = 0;;i++) {
        MergeableSection *m = file->mergeable_sections.data[i];
        if (m == NULL)
            break;
        if (m) {
            VectorNew(&(m->fragments),m->strings.size);
            for(i64 i = 0;i < m->strings.size;i++) {
                StringView *temp = (StringView *)m->strings.data;
                VectorAdd(&(m->fragments),merge_sec_insert(ctx,(StringView *)m->strings.data[i],(u64)m->hashes.data[i],
                                                 m->p2align,m->parent),sizeof(SectionFragment *));
            }
        }
    }
     

    // Attach section pieces to symbols.
    for (i64 i = 1;i < file->inputfile.elf_syms.size; i++) {
        if (file->inputfile.symbols.data == NULL)
            break;
        ELFSymbol *sym = (ELFSymbol *)file->inputfile.symbols.data[i];
        ElfSym *esym = file->inputfile.elf_syms.data[i];
        if (esym == NULL)
            break;
        if (is_abs(esym) || is_common(esym) || is_undef(esym))
            continue;
        MergeableSection *m = file->mergeable_sections.data[get_shndx(esym)];
        if (!m || m->fragments.size == 0) 
            continue;
        
        SectionFragment *frag;
        i64 frag_offset;
        frag = get_fragment(*esym->st_value.val,m);
        if (!frag)
            Fatal(": bad symbol value: ");
        ELFSym_set_frag(frag,sym);
        sym->value = frag_offset;
    }

    // Compute the size of frag_syms.
    i64 nfrag_syms = 0;
    for (int i = 0;;i++) {
        if (file->sections == NULL)
            break;
        InputSection *isec = file->sections[i];
        if (isec == NULL) 
            break;
        if (isec && isec->is_alive && (*get_shdr(file,i)->sh_flags.val & SHF_ALLOC)) {
            // for(int j = 0;;j++) {
            //     ElfRel *r = get_rels();
            // } 0
        }
    }
    VectorNew(&(file->inputfile.frag_syms),nfrag_syms);
    i64 idx = 0;
    for (int i = 0;;i++) {
        if (file->sections == NULL)
            break;
        InputSection *isec = file->sections[i];
        if (isec == NULL) 
            break;
        if (!isec && !isec->is_alive && !(*get_shdr(file,isec->shndx)->sh_flags.val & SHF_ALLOC)) {
            // for(int j = 0;;j++) {
            //     ElfRel *r = get_rels();
            // }
        }
    }
}

void obj_resolve_symbols(Context *ctx,ObjectFile *obj) {
  for (i64 i = obj->inputfile.first_global; i < obj->inputfile.elf_syms.size; i++) {
    ELFSymbol *sym = (ELFSymbol *)obj->symbols.data[i];
    ElfSym *esym =  (ElfSym *)obj->inputfile.elf_syms.data[i];
    if (is_undef(esym))
      continue;

    InputSection *isec = NULL;
    if (!is_abs(esym) && !is_common(esym)) {
      isec = get_section(esym,obj);
      if (!isec || !isec->is_alive)
        continue;
    }
    u64 left = get_rank(&(obj->inputfile), esym, !obj->inputfile.is_alive);
    u64 right = get_rank_single_sym(sym,i);
    if (get_rank(&(obj->inputfile), esym, !obj->inputfile.is_alive) < get_rank_single_sym(sym,i)) {
      sym->file = obj;
      set_input_section(isec,sym);
      sym->value = *esym->st_value.val;
      sym->sym_idx = i;
      sym->ver_idx = ctx->default_version;
      sym->is_weak = is_weak(esym);
    }
  }
}

void obj_scan_relocations(Context *ctx,ObjectFile *file) {
    for(int i = 0;i < file->inputfile.elf_sections_num;i++) {
        if (file == NULL || file->sections == NULL)
            break;
        InputSection *isec = file->sections[i];
        if (isec == NULL)
            break;
        if (isec && isec->is_alive && (*get_shdr(file,i)->sh_flags.val & SHF_ALLOC)) {
            isec_scan_relocations(ctx,file,i,isec);
        }
    }
}

void scan_relocations(Context *ctx) {
    for (int i = 0;i < ctx->objs.size;i++) {
        ObjectFile *file = (ObjectFile *)ctx->objs.data[i];
        obj_scan_relocations(ctx,file);
    }
}

static bool should_write_to_local_symtab(Context *ctx, ELFSymbol *sym,int i,ObjectFile *file) {
    if (elfsym_get_type(file,i + 1) == STT_SECTION)
        return false;
    if (starts_with(sym->nameptr,".L")) {
        if (ctx->arg.discard_locals)
        return false;
        InputSection *isec = elfsym_get_input_section(sym);
        if (isec)
            if (*get_shdr(file,i)->sh_flags.val & SHF_MERGE)
                return false;
    }

    return true;
}

bool is_alive(ELFSymbol *sym) {
    SectionFragment *frag = get_frag(sym);
    if (frag)
        return frag->is_alive;
    InputSection *isec = elfsym_get_input_section(sym);
    if (isec)
        return isec->is_alive;
    return true;
}
void obj_compute_symtab_size(Context *ctx,ObjectFile *file) {
    if(ctx->arg.strip_all)
        return;
    VectorNew(&(file->inputfile.output_sym_indices),file->inputfile.elf_syms.size);
    
    // Compute the size of local symbols
    if (!ctx->arg.discard_all && !ctx->arg.strip_all && !ctx->arg.retain_symbols_file.size) {
        for(i64 i = 0;i + 1 < file->inputfile.first_global;i++) {
            ELFSymbol *sym = file->symbols.data[i + 1];

            if (is_alive(sym) && should_write_to_local_symtab(ctx, sym,i,file)) {
                file->inputfile.strtab_size += sym->namelen + 1;
                i32* cin = (i32*)malloc(sizeof(i32));
                *cin = file->inputfile.num_local_symtab++;
                VectorAdd(&(file->inputfile.output_sym_indices),cin,sizeof(i32 *));
                sym->write_to_symtab = true;
            }
        }
    }

    // Compute the size of global symbols.
    for(i64 i = file->inputfile.first_global;i < file->inputfile.elf_syms.capacity;i++) {
        ELFSymbol *sym = file->symbols.data[i];
        if (sym && sym->file == file && is_alive(sym) && 
            (!ctx->arg.retain_symbols_file.size || sym->write_to_symtab)) {
            file->inputfile.strtab_size += sym->namelen + 1;
            i32* cin = (i32*)malloc(sizeof(i32));
            if (is_local(ctx,sym,file,i)) 
                *cin = file->inputfile.num_local_symtab++;
            else 
                *cin = file->inputfile.num_global_symtab++;
            
            VectorAdd(&(file->inputfile.output_sym_indices),cin,sizeof(i32 *));
            sym->write_to_symtab = true;    
        }
    }
}

void write_sym(ELFSymbol *sym, i64 idx,Context *ctx,ElfSym *symtab_base,u8 *strtab_base,i64 *strtab_off,ObjectFile *file) {
    U32 *xindex = NULL;
    symtab_base[idx] = to_output_esym(ctx, sym, *strtab_off, xindex,file);
    *strtab_off += write_string(strtab_base + *strtab_off, sym->nameptr);
};

void obj_populate_symtab(Context *ctx,ObjectFile *file) {
    ElfSym *symtab_base = (ElfSym *)(ctx->buf + *(u32 *)&(ctx->symtab->chunk->shdr.sh_offset));

    u8 *strtab_base = ctx->buf + *(u32 *)&(ctx->strtab->chunk->shdr.sh_offset);
    i64 *strtab_off = (i64 *)malloc(sizeof(i64));
    *strtab_off = file->inputfile.strtab_offset;

    i64 local_symtab_idx = file->inputfile.local_symtab_idx;
    i64 global_symtab_idx = file->inputfile.global_symtab_idx;

    for (i64 i = 1; i < file->inputfile.first_global; i++) {
        ELFSymbol *sym = file->symbols.data[i];
        if (sym->write_to_symtab)
            write_sym(sym, local_symtab_idx++,ctx,symtab_base,strtab_base,strtab_off,file);
    }
    
    for (i64 i = file->inputfile.first_global;i < file->inputfile.elf_syms.size;i++) {
        ELFSymbol *sym = file->symbols.data[i];
        if(sym->file == file && sym->write_to_symtab) {
            if (is_local(ctx,sym,file,i)) 
                write_sym(sym, local_symtab_idx++,ctx,symtab_base,strtab_base,strtab_off,file);
            else 
                write_sym(sym, global_symtab_idx++,ctx,symtab_base,strtab_base,strtab_off,file);
        }
    }
}

void file_convert_common_symbols(Context *ctx,ObjectFile *obj) {
    if (!obj->has_common_symbol)
        return;
}