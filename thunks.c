#include "mold.h"
static  i64 max_thunk_size = 102400;

i64 d_thunk_end(i64 offset,vector m,i64 d){
    u64 d_end = align_to(offset, 1 << ((InputSection *)m.data[d])->p2align) + ((InputSection *)m.data[d])->sh_size;
    return align_to(d_end, 16) + max_thunk_size;
};

static i64 max_distance() {
    if (!strcmp(target.target_name,"arm32"))
        return 1 << 24;
    return 1 << 25;
}

i64 thunk_size(Thunk *thunk) {
    return target.thunk_hdr_size + thunk->symbols.size * target.thunk_size; 
}
Thunk *create_thunk(OutputSection *osec,i64 offset) {
    Thunk *thunk = (Thunk *)malloc(sizeof(Thunk));
    thunk->output_section = osec;
    thunk->offset = offset;
    return thunk;
}

static void reset_thunk(Thunk *thunk) {
    if (thunk->symbols.size == 0)
        return;
    for (int i = 0;i < thunk->symbols.size;i++) {
        ELFSymbol *sym = thunk->symbols.data[i];
        sym->extra.thunk_idx = -1;
        sym->extra.thunk_sym_idx = -1;
        sym->flags = 0;
    }
}
void create_range_extension_thunks(Context *ctx,OutputSection *osec) {
    i64 batch_size = max_distance() / 10;
    vector m = osec->member;
    if(!m.size)
        return;
    for(int i = 0;i < m.size;i++) {
        InputSection *isec = m.data[i];
        isec->offset = -1;
    }
    i64 a = 0;
    i64 b = 0;
    i64 c = 0;
    i64 d = 0;
    i64 offset = 0;

    // The smallest thunk index that is reachable from the current batch.
    i64 t = 0;
    while (b < m.size) {
         while (d < m.size &&
                (b == d || d_thunk_end(offset,m,d) <= ((InputSection *)m.data[b])->offset + max_distance())) {
            offset = align_to(offset, 1 << ((InputSection *)m.data[d])->p2align);
            ((InputSection *)m.data[d])->offset = offset;
            offset += ((InputSection *)m.data[d])->sh_size;
            d++;
        }

        c = b + 1;
        // Move C forward so that C is apart from B by BATCH_SIZE. We want
        // to make sure that there's at least one section between B and C
        // to ensure progress.
        while (c < d && ((InputSection *)m.data[c])->offset + ((InputSection *)m.data[c])->sh_size < ((InputSection *)m.data[b])->offset + batch_size)
            c++;
        
        // Move A forward so that A is reachable from C.
        i64 c_offset = (c == m.size) ? offset : ((InputSection *)m.data[c])->offset;
        while (a < b && ((InputSection *)m.data[a])->offset + max_distance() < c_offset)
            a++;

        // Create a new thunk and place it at D.
        offset = align_to(offset, 16);
        if (osec->thunks.data == NULL)
            VectorNew(&(osec->thunks),1);
        i64 thunk_idx = osec->thunks.size;
        Thunk *thunk = create_thunk(osec, offset);
        VectorAdd(&(osec->thunks),thunk,sizeof(Thunk *));

        // Scan relocations between B and C to collect symbols that need
        // entries in the new thunk.
        for(int i = b;i < c;i++) {
            // InputSection *isec = (InputSection *)m.data[i];
            // 暂时没有
            // scan_rels(ctx, isec, thunk, thunk_idx);
        }

        assert(thunk_size(thunk) < max_thunk_size);

        // Sort symbols added to the thunk to make the output deterministic.
        // 暂时没有
        // Assign offsets within the thunk to the symbols.

        b = c;
    }
    while (t < osec->thunks.size)
        reset_thunk(osec->thunks.data[t++]);
    *osec->chunk->shdr.sh_size.val = offset;

    for (int i = 0;i < osec->member.size;i++) {
        InputSection *isec = (InputSection *)osec->member.data[i];
        *osec->chunk->shdr.sh_addralign.val = 
            *osec->chunk->shdr.sh_addralign.val > 1 << isec->p2align ? *osec->chunk->shdr.sh_addralign.val : 1 << isec->p2align;
    }
}