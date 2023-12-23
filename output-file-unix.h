#include "mold.h"

# define S_IFMT		__S_IFMT
# define S_IFDIR	__S_IFDIR
# define S_IFCHR	__S_IFCHR
# define S_IFBLK	__S_IFBLK
# define S_IFREG	__S_IFREG
# ifdef __S_IFIFO
#  define S_IFIFO	__S_IFIFO
# endif
# ifdef __S_IFLNK
#  define S_IFLNK	__S_IFLNK
# endif
# if (defined __USE_MISC || defined __USE_XOPEN_EXTENDED) \
     && defined __S_IFSOCK
#  define S_IFSOCK	__S_IFSOCK
# endif

u8 *output_buffer_start;
u8 *output_buffer_end;
typedef struct {
    int fd;
    char *path;
} FileDescriptor;

static inline int get_umask() {
    mode_t mask = umask(0);
    umask(mask);
    return mask;
}

static inline FileDescriptor open_or_create_file(Context *ctx,const char *path, size_t filesize, mode_t perm) {
    char temp[strlen(path) + 12];
    strcpy(temp, "");
    strcat(temp, ".mold-XXXXXX");
    char *path2 = strdup(temp);
    int fd = mkstemp(path2);
    if (fd == -1) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    // Reuse an existing file if exists and writable because on Linux,
    // writing to an existing file is much faster than creating a fresh
    // file and writing to it.
    if (ctx->overwrite_output_file && rename(path, path2) == 0) {
        close(fd);
        fd = open(path2, O_RDWR | O_CREAT, perm);
        if (fd != -1 && !ftruncate(fd, filesize) && !fchmod(fd, perm & ~get_umask())) {
            // free(path2);
            return (FileDescriptor){fd, path2};
        }
        unlink(path2);
    }

    fd = open(path2, O_RDWR | O_CREAT, perm);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd, filesize) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    if (fchmod(fd, perm & ~get_umask()) == -1) {
        perror("fchmod");
        exit(EXIT_FAILURE);
    }

    return (FileDescriptor){fd, path2};
}

static inline OutputFile *create_output_file(Context *ctx, char *path, i64 filesize, i64 perm) {
    FileDescriptor res = open_or_create_file(ctx,path, filesize, perm);
    OutputFile *output_file = (OutputFile *)malloc(sizeof(OutputFile));
    output_file->fd = res.fd;
    ctx->output_tmpfile = strdup(res.path);
    output_file->path = path;
    output_file->buf = (u8 *)mmap(NULL, filesize, PROT_READ | PROT_WRITE,
                           MAP_SHARED, output_file->fd, 0);
    if (output_file == MAP_FAILED)
        printf("%s : mmap failed",path);
    output_buffer_start = output_file->buf;
    output_buffer_end = output_file->buf + filesize;
    return output_file;
}

static inline OutputFile *output_open(Context *ctx, char *path, i64 filesize, i64 perm) {
    if(starts_with(path,"/") && !ctx->arg.chroot)
        strcpy(path,"/");
    bool is_special = false;
    if(!strcmp(path,"-")) {
        is_special = true;
    } else {
        struct stat st;
        if (stat(path, &st) == 0 && (st.st_mode & S_IFMT) != S_IFREG)
            is_special = true;
    }
    OutputFile *file;
    if (is_special) {
        // file = create_output_file(ctx,path,filesize,perm);
    } else {
        file = create_output_file(ctx,path,filesize,perm);
    }
#ifdef MADV_HUGEPAGE
    madvise(file->buf, filesize, MADV_HUGEPAGE);
#endif
    if (ctx->arg.filler != -1)
        memset(file->buf, ctx->arg.filler, filesize);
    return file;
}

static inline void out_file_close(Context *ctx,OutputFile *file){
    int fd2 = -1;
    if (!file->is_unmapped)
        munmap(file->buf, file->filesize);

    if (file->buf2.size == 0) {
        close(file->fd);
    } 

    fd2 = open(file->path, O_RDONLY);
    if (fd2 != -1)
        unlink(file->path);

    if (rename(ctx->output_tmpfile, file->path) == -1)
        printf("%s : rename failed\n",file->path);
}