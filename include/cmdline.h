#ifndef CMDLINE 
#define CMDLINE 
#include "common.h"
static inline void append(char*** vec, const char* str) {
    int size = 0;
    if (*vec != NULL) {
        while ((*vec)[size] != NULL) {
            size++;
        }
    }

    *vec = (char **)realloc(*vec, (size + 2) * sizeof(char*));
    (*vec)[size] = strdup(str);
    (*vec)[size + 1] = NULL;
}

static inline char ** expand_response_files(Context *ctx,char **argv) {
    char** vec = NULL;
    int i = 0;

    while (argv[i] != NULL) {
        if (argv[i][0] == '@') {
            // 模拟 read_response_file 函数
            append(&vec, argv[i] + 1);
        } else {
            append(&vec, argv[i]);
        }
        i++;
    }

    return vec;
}

static inline char ** parse_nonpositional_args(Context *ctx) {
    char ** args = ctx->cmdline_args;
    args = args + 1; 

    char **remaining = NULL;
    char *arg;
    ctx->arg.color_diagnostics = true;
    bool version_shown = false;
    bool warn_shared_textrel = false;
    int count = 0;
    while (*args != NULL) {
        
        if (!strcmp(*args,"-o")) {
            args = args + 1; 
            arg = *args;
            ctx->arg.output = strdup(arg);
        } else if (!strcmp(*args,"--no-fork")) {
            ctx->arg.fork = false;
        } else {
            if (args[0][0] == '-')
                fprintf(stderr, "Fatal: %s\n", *args);
            count++;
            char **temp = (char **)realloc(remaining, count * sizeof(char *));
            remaining = temp;
            remaining[count - 1] = strdup(*args);
        }
        args = args + 1; 
    }

    if (ctx->arg.thread_count == 0)
        ctx->arg.thread_count = 2;
    
    ctx->arg.undefined_count = 0;
    push_back(ctx,ctx->arg.entry);
    return remaining;
}

static inline void free_vec(char** vec) {
    if (vec != NULL) {
        for (int i = 0; vec[i] != NULL; i++) {
            free(vec[i]);
        }
        free(vec);
    }
}

#endif