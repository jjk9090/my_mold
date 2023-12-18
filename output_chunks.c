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
    bool inserted;
    //   update_maximum(frag->p2align, p2align);
    return frag;
}