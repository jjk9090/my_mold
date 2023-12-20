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
    merger_sec_insert_element(sec,data->data,frag/*,hash*/);
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