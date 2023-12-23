#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct {
    int fd;
    char *path;
} FileDescriptor;

int get_umask() {
    mode_t mask = umask(0);
    umask(mask);
    return mask;
}

FileDescriptor open_or_create_file(const char *path, size_t filesize, mode_t perm) {
    char *path2 = malloc(strlen(path) + 12);
    strcpy(path2, path);
    strcat(path2, ".mold-XXXXXX");

    int fd = mkstemp(path2);
    if (fd == -1) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    // Reuse an existing file if exists and writable because on Linux,
    // writing to an existing file is much faster than creating a fresh
    // file and writing to it.
    if (rename(path, path2) == 0) {
        close(fd);
        fd = open(path2, O_RDWR | O_CREAT, perm);
        if (fd != -1 && !ftruncate(fd, filesize) && !fchmod(fd, perm & ~get_umask())) {
            free(path2);
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

int main() {
    const char *path = "/home/jjk/project/mold2/my_mold/test_demo/tt4/main.o";
    size_t filesize = 1024;  // 你的文件大小
    mode_t perm = 0666;  // 文件权限，比如读写权限
    FileDescriptor output_file = open_or_create_file(path, filesize, perm);

    // 使用 output_file.fd 进行读写操作

    close(output_file.fd);
    unlink(output_file.path);
    free(output_file.path);

    return 0;
}
