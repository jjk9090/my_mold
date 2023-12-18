#ifndef MAPPEDFILE_H
#define MAPPEDFILE_H

#include "mold.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
# include <sys/mman.h>

typedef struct MappedFile {
    char* name;
    u8 * data;
    long long size;
    bool given_fullpath;
    struct MappedFile* parent;
    struct MappedFile* thin_parent;
    int fd;
#ifdef _WIN32
    HANDLE file_handle;
#endif
} MappedFile;

static inline MappedFile* mapped_file_open(Context* ctx, const char* path) {
    MappedFile* mf = (MappedFile*)malloc(sizeof(MappedFile));
    if (mf == NULL) {
        // 处理内存分配失败的情况
        return NULL;
    }
    mf->name = strdup(path);
    mf->data = NULL;
    mf->size = 0;
    mf->given_fullpath = 1;
    mf->parent = NULL;
    mf->thin_parent = NULL;
    mf->fd = -1;
#ifdef _WIN32
    mf->file_handle = INVALID_HANDLE_VALUE;
#endif
    // 执行打开文件并映射等操作
    return mf;
}

static inline void mapped_file_unmap(MappedFile* mf) {
    // 执行释放映射和关闭文件等操作
}

static inline MappedFile* mapped_file_slice(Context* ctx, MappedFile* mf, const char* name, long long start, long long size) {
    // 创建一个新的 MappedFile 结构体，并在其中设置必要的成员变量值
    // return new_mf;
}

static inline const char* mapped_file_get_contents(MappedFile* mf) {
    return (const char*)mf->data;
}

static inline long long mapped_file_get_offset(MappedFile* mf) {
    return mf->parent ? (mf->data - mf->parent->data + mapped_file_get_offset(mf->parent)) : 0;
}

static inline char* mapped_file_get_identifier(MappedFile* mf) {
    if (mf->parent) {
        // 如果有父级 MappedFile，则使用偏移量作为唯一标识符
        char* identifier = (char*)malloc(strlen(mf->parent->name) + 1 + 20);
        if (identifier == NULL) {
            // 处理内存分配失败的情况
            return NULL;
        }
        sprintf(identifier, "%s:%lld", mf->parent->name, mapped_file_get_offset(mf));
        return identifier;
    }

    if (mf->thin_parent) {
        // 如果是一个thin_archive，使用文件名作为唯一标识符
        char* identifier = (char*)malloc(strlen(mf->thin_parent->name) + 1 + strlen(mf->name) + 1);
        if (identifier == NULL) {
            // 处理内存分配失败的情况
            return NULL;
        }
        sprintf(identifier, "%s:%s", mf->thin_parent->name, mf->name);
        return identifier;
    }

    // 如果没有父级和thin_archive，则使用文件名作为唯一标识符
    return strdup(mf->name);
}

static inline void mapped_file_free(MappedFile* mf) {
    if (mf != NULL) {
        free(mf->name);
        free(mf->data);
        free(mf);
    }
}

static inline MappedFile *MappedFile_open(Context *ctx,char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        if (errno != ENOENT)
            fprintf(stderr, "opening %s failed: %s\n", path, strerror(errno));
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "%s: fstat failed: %s\n", path, strerror(errno));
        close(fd);
        return NULL;
    }

    MappedFile* mf = (MappedFile*)malloc(sizeof(MappedFile));
    // mf_pool
    if (mf == NULL) {
        fprintf(stderr, "Failed to allocate memory for MappedFile\n");
        close(fd);
        return NULL;
    }

    mf->name = strdup(path);
    mf->size = st.st_size;

    if (st.st_size > 0) {
#ifdef _WIN32
        HANDLE handle = CreateFileMapping((HANDLE)_get_osfhandle(fd),
                                          NULL, PAGE_READWRITE, 0,
                                          st.st_size, NULL);
        if (!handle) {
            fprintf(stderr, "%s: CreateFileMapping failed\n", path);
            close(fd);
            free(mf->name);
            free(mf);
            return NULL;
        }
        mf->file_handle = handle;
        mf->data = (unsigned char*)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, st.st_size);
        if (!mf->data) {
            fprintf(stderr, "%s: MapViewOfFile failed\n", path);
            CloseHandle(handle);
            close(fd);
            free(mf->name);
            free(mf);
            return NULL;
        }
#else
        mf->data = (unsigned char*)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE, fd, 0);
        if (mf->data == MAP_FAILED) {
            fprintf(stderr, "%s: mmap failed\n", path);
            close(fd);
            free(mf->name);
            free(mf);
            return NULL;
        }
#endif
    }

    close(fd);
    return mf;
}

static inline StringView get_contents(MappedFile *mf) {
    const char* data = mf->data;
    size_t size = strlen(data);
    return (StringView){data, size};
}

static inline MappedFile* mapped_file_must_open(Context* ctx, char* path) {
    MappedFile* mf = MappedFile_open(ctx, path);
    if (mf == NULL) {
        fprintf(stderr, "cannot open a file: %s\n",strerror(errno));   
    }
    return mf;
}

#endif