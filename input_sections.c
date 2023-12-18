#include "mold.h"

int64_t to_p2align(uint64_t alignment) {
    if (alignment == 0)
        return 0;

    int64_t count = 0;
    while ((alignment & 1) == 0) {
        alignment >>= 1;
        count++;
    }

    return count;
}

void input_sec_uncompress(Context *ctx,ObjectFile *file,int i) {
    if (!(*get_shdr(file,i)->sh_flags.val & SHF_COMPRESSED) || file->sections[i]->uncompressed)
        return;

    // u8* buf = (u8*)malloc(file->sections[i]->sh_size * sizeof(u8));
    // uncompress_to(ctx, buf);
    // contents = std::string_view((char *)buf, sh_size);
    // ctx.string_pool.emplace_back(buf);
    // uncompressed = true;
}
void define_input_section(ObjectFile *file,Context *ctx,int i) {
    if (i < file->inputfile.elf_sections_num) {
        file->sections[i]->contents.data = (char *)file->inputfile.inputfile_mf->data + *get_shdr(file,i)->sh_offset.val;
        file->sections[i]->contents.size = (size_t)(*get_shdr(file,i)->sh_size.val);
    }
    const char* startAddress = file->sections[i]->contents.data;
    // size_t size = file->sections[i]->contents.size;
    // char* buffer = malloc(size);
    // memcpy(buffer, startAddress, size);
    // printf("%s\n", buffer);
    InputSection *section_t = (InputSection *)malloc(sizeof(InputSection));
    file->sections[i]->sh_size = *get_shdr(file,i)->sh_size.val;
    file->sections[i]->shndx = i;
    file->sections[i]->file = (ObjectFile *)malloc(sizeof(ObjectFile));
    file->sections[i]->file = file;
    file->sections[i]->p2align = to_p2align(*get_shdr(file,i)->sh_addralign.val);
    section_t = file->sections[i];

    if (!target.is_rela) {
        input_sec_uncompress(ctx,file,i);
    }
    
}