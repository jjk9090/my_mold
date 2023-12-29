#include "mold.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
ARM32 target;

StringView get_string(Context *ctx, ElfShdr *shdr) {
    // printf("%ld\n",sizeof(shdr->sh_offset.val));
    uint16_t value = 0;
    // value = shdr->sh_offset.value;
    value = (shdr->sh_offset.val[3] << 24)| (shdr->sh_offset.val[2] << 16) | (shdr->sh_offset.val[1] << 8) | shdr->sh_offset.val[0];
    // 0x7ffff7fc80a8 "" begin
    // // 将 value 格式化为十六进制字节
    // // printf("Value in hexadecimal: 0x%04X\n", value);
    // ElfShdr *sh_begin = (ElfShdr *)((unsigned char*)mf->data + value);
    u8 *begin = inputfile_mf->data + value;
    u8 *end = begin + *shdr->sh_size.val;
    if (inputfile_mf->data + inputfile_mf->size < end)
        fprintf(stderr,"%s:section header is out of range: %d",strerror(errno),*shdr->sh_offset.val);
    StringView result;
    result.data = (const char*)begin;
    result.size = (size_t)(end - begin);
    return result;
}

// 获取elf文件类型
char *get_elf_type (u8 *buf) {
    bool is_le = (((ElfEhdr *)buf)->e_ident[EI_DATA] == ELFDATA2LSB);
    bool is_64;
    u16 e_machine;
    ElfEhdr *test = (ElfEhdr *)buf;

    if (is_le) {
        is_64 = (((ElfEhdr *)buf)->e_ident[EI_CLASS] == ELFCLASS64);
        e_machine = *((ElfEhdr *)buf)->e_machine.val;
    }
    
    switch (e_machine) {
        case EM_ARM:
            target.target_name = strdup("arm32");
            target.is_64 = false;
            target.is_le = true;
            target.is_rela = false;
            target.page_size = 4096;
            target.e_machine = EM_ARM;
            target.plt_hdr_size = 32;
            target.plt_size = 16;
            target.pltgot_size = 16;
            target.thunk_hdr_size = 16;
            target.thunk_size = 16;
            target.filler[0] = 0xff;
            target.filler[1] = 0xde;
            target.R_COPY = R_ARM_COPY;
            target.R_GLOB_DAT = R_ARM_GLOB_DAT;
            target.R_JUMP_SLOT = R_ARM_JUMP_SLOT;
            target.R_ABS = R_ARM_ABS32;
            target.R_RELATIVE = R_ARM_RELATIVE;
            target.R_IRELATIVE = R_ARM_IRELATIVE;
            target.R_DTPOFF = R_ARM_TLS_DTPOFF32;
            target.R_TPOFF = R_ARM_TLS_TPOFF32;
            target.R_DTPMOD = R_ARM_TLS_DTPMOD32;
            target.R_TLSDESC = R_ARM_TLS_DESC;
            
            uint32_t funcall[] = {R_ARM_JUMP24, R_ARM_THM_JUMP24, R_ARM_CALL, R_ARM_THM_CALL, R_ARM_PLT32};
            for (int i = 0; i < ARRAY_SIZE(target.R_FUNCALL); ++i) {
                target.R_FUNCALL[i] = funcall[i];
            }
            return "arm32";
        default:
            return "";
    }
}

char *get_machine_type(Context *ctx,MappedFile *mf) {
    StringView data = get_contents(mf);
    switch (get_file_type(ctx, mf)) {
        case ELF_OBJ:
        case ELF_DSO:
            return get_elf_type(mf->data);
    }
}

// 获取目标文件target
char *deduce_machine_type (Context *ctx,char **args) {
    MappedFile * mf = MappedFile_open(ctx,*args);
    char *target  = strdup(get_machine_type(ctx, mf));
    printf("target: %s\n",target);
    return target;
}

static void
check_file_compatibility(Context *ctx, MappedFile *mf) {
    char  *target = strdup(get_machine_type(ctx, mf));

    if (strcmp(target,ctx->arg.emulation))
        printf("%s: incompatible file type: %s is expected but got %s\n", mf->name, ctx->arg.emulation, target);
}

ObjectFile *new_object_file (Context *ctx,MappedFile *mf,char *archive_name) {
    check_file_compatibility(ctx, mf);
    bool in_lib = ctx->in_lib;

    ObjectFile *file = objectfile_create(ctx, mf, archive_name, in_lib);
    file->inputfile.filename = strdup(mf->name);
    file->inputfile.priority = ctx->file_priority++;
    file->inputfile.is_alive = !in_lib;
    parse(ctx,file);
    return file;
}

// 读取文件
void read_file(Context *ctx,MappedFile *mf) {
    switch (get_file_type(ctx, mf)) {
        case ELF_OBJ:
            // new_object_file(ctx, mf, "");
            VectorAdd(&(ctx->objs),new_object_file(ctx, mf, ""),sizeof(ObjectFile *));
            return;
    }
}

static void read_input_files (Context *ctx,char **args) {
    ctx->is_static = ctx->arg.is_static;
    VectorNew(&(ctx->mf_pool),1);
    VectorNew(&(ctx->objs), 1);
    while (*args != NULL)  {
        char *arg = args[0];
        if (!strcmp(arg,"--as-needed")) {

        } else {
            MappedFile *mf = mapped_file_must_open(ctx, arg);
            VectorAdd(&(ctx->mf_pool),mf,sizeof(MappedFile *));
            read_file(ctx, mf);
        }
        args = args + 1;
    }
}

char *get_mold_version() {
    char *name = MOLD_IS_SOLD ? "mold (sold) " : "mold ";
    char *versionString = (char *)malloc(100 * sizeof(char));  // 分配足够大的内存空间

    sprintf(versionString, "%s%s (%s; compatible with GNU ld)", name, MOLD_VERSION, mold_git_hash);
    return versionString;
}

void init_context(Context *ctx) {
    VectorNew(&(ctx->arg.section_order),1);
    VectorNew(&(ctx->chunks),1);
    VectorNew(&(ctx->osec_pool),1);
    VectorNew(&(ctx->string_pool),1);
}
void init_ctx(Context *ctx) {
    ctx->arg.entry = get_symbol(ctx,"_start");
    ctx->arg.fini = get_symbol(ctx,"_fini");
    ctx->arg.init = get_symbol(ctx,"_init");
    ctx->arg.is_static = false;
    ctx->in_lib = false;
    
    ctx->file_priority = 10000;
    ctx->default_version = VER_NDX_UNSPECIFIED;
    ctx->dtp_addr = 0;
    ctx->overwrite_output_file = true;

    ctx->arg.relocatable = false;
    ctx->arg.oformat_binary = false;
    ctx->arg.relocatable_merge_sections = false;
    ctx->arg.gc_sections = false;
    ctx->arg.z_sectionheader = true;
    ctx->arg.z_now = false;
    ctx->arg.eh_frame_hdr = true;
    ctx->arg.z_separate_code =  NOSEPARATE_CODE;
    ctx->arg.start_stop = false;
    ctx->arg.hash_style_sysv = true;
    ctx->arg.hash_style_gnu = true;
    ctx->arg.z_relro = true;
    ctx->arg.discard_all = false;
    ctx->arg.strip_all = false;
    ctx->arg.discard_locals = false;
    ctx->arg.omagic = false;
    ctx->arg.rosegment = true;
    ctx->arg.z_stack_size = 0;
    // u64 image_base = 0x200000;
    ctx->arg.image_base = 0x200000;
    ctx->merged_sections_count = 0;
    ctx->arg.execute_only = false;
    ctx->arg.nmagic = false;
    ctx->arg.filler = -1;
    ctx->arg.pic = false;
    ctx->arg.quick_exit = true;
    ctx->arg.dynamic_linker = NULL;
    ctx->arg.hash_style_sysv = true;
    VectorNew(&(ctx->arg.retain_symbols_file),1);
    ctx->gloval_merge_sec = (MergedSection *)malloc(sizeof(MergedSection));
    ctx->gloval_merge_sec->map = NULL;
    VectorNew(&merge_string,1);
}
int main(int argc, char **argv) {
    mold_version = get_mold_version();
    // 创建保存上下文信息的结构体
    Context ctx;
    (&ctx)->symbol_map = NULL;
    insert_symbol(&ctx,"_start", "_start");
    insert_symbol(&ctx,"_fini","_fini");
    insert_symbol(&ctx,"_init","_init");
    VectorNew(&((&ctx)->merged_sections),1);
    // 解析拓展命令行参数
    ctx.cmdline_args = expand_response_files(&ctx,argv);
    
    init_ctx(&ctx);
    
    init_context(&ctx);
    
    // VectorNew(&(&ctx)->string_pool,1);
    // 使用展开后的参数列表
    // printf("Expanded arguments:\n");
    // for (int i = 0; ctx.cmdline_args[i] != NULL; i++) {
    //     printf("%s\n", ctx.cmdline_args[i]);
    // }
    // 
    char **file_args = parse_nonpositional_args(&ctx);
        ctx.arg.emulation = strdup(deduce_machine_type(&ctx, file_args));
    
    // if (!strcmp(ctx.arg.emulation,"x86_64"))
    //     return redo_main(ctx, argc, argv);
    // 读取输入文件
    read_input_files(&ctx, file_args);

    ctx.page_size = target.page_size;

    if (!ctx.arg.relocatable)
        create_internal_file(&ctx);
    
    // 解析符号
    resolve_symbols(&ctx);

    // merge sections
    resolve_section_pieces(&ctx);

    // Create .bss sections for common symbols.
    convert_common_symbols(&ctx);

    // 创造.comment 段
    compute_merged_section_sizes(&ctx);

    create_synthetic_sections(&ctx);

    // 最简单的例子暂时不需要
    // if (!ctx.arg.allow_multiple_definition)
    // check_duplicate_symbols(ctx);

    // check_symbol_types(ctx);

    // 创造输出段 Bin input sections into output sections.
    create_output_sections(&ctx);

    // 添加一些synthetic的符号
    add_synthetic_symbols(&ctx);

    // 扫描重定位
    scan_relocations(&ctx);

    // Compute sizes of output sections while assigning offsets
    // within an output section to input sections.  size
    compute_section_sizes(&ctx);

    sort_output_sections(&ctx);
    // ctx.dynsym->finalize(ctx);  暂时没有
    
    // Fill .gnu.version_r section contents.
    // 对verneed段进行构造，实际写入内容。其中包含了字符串信息，因此还会将字符串写入dynstr中
    verneed_construct(&ctx);

    // Compute .symtab and .strtab sizes for each file.
    create_output_symtab(&ctx);

    // 构造output .eh_frame
    eh_frame_construct(&ctx);
    for(int i = 0;i < (&ctx)->chunks.size;i++) {
        Chunk *chunk = (Chunk *)((&ctx)->chunks.data[i]);
        if(!strcmp(chunk->name,".text"))
            printf("%s\n",chunk->name);
    }
    // Compute the section header values for all sections.
    compute_section_headers(&ctx);

    // Assign offsets to output sections
    i64 filesize = set_osec_offsets(&ctx);
    for(int i = 0;i < (&ctx)->chunks.size;i++) {
        Chunk *chunk = (Chunk *)((&ctx)->chunks.data[i]);
        if(!strcmp(chunk->name,".text"))
            printf("%s\n",chunk->name);
    }
    // Set actual addresses to linker-synthesized symbols.
    fix_synthetic_symbols(&ctx);

    // Create an output file
    (&ctx)->output_file = output_open(&ctx, ctx.arg.output, filesize, 0777);
    ctx.buf = ctx.output_file->buf;
    
    for(int i = 0;i < merge_string.size;i++) {
        printf("%s\n",(char *)merge_string.data[i]);
    }
    // Copy input sections to the output file and apply relocations.
    copy_chunks(&ctx);

    // 关闭文件
    out_file_close(&ctx,(&ctx)->output_file);

    if (ctx.arg.quick_exit)
        _exit(0);

    return 0;
}