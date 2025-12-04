#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <link.h>
#include <sys/mman.h>

// Phase 1.3: /proc filesystem hiding via LD_PRELOAD

static FILE* (*real_fopen)(const char*, const char*) = NULL;
static FILE* (*real_fopen64)(const char*, const char*) = NULL;
static int (*real_open)(const char*, int, ...) = NULL;

static void init_hooks() __attribute__((constructor));

static void init_hooks() {
    real_fopen = dlsym(RTLD_NEXT, "fopen");
    real_fopen64 = dlsym(RTLD_NEXT, "fopen64");
    real_open = dlsym(RTLD_NEXT, "open");
}

// Filter /proc reads to hide frida artifacts
static int should_filter(const char *path) {
    if (!path) return 0;
    
    // Hide specific proc entries
    const char *filtered_paths[] = {
        "/proc/self/maps",      // Hide memory mappings
        "/proc/self/status",    // Hide tracer info
        "/proc/self/cmdline",   // Hide command line
        "/proc/net/tcp",        // Hide network connections
        "/proc/net/tcp6",
        NULL
    };
    
    for (int i = 0; filtered_paths[i]; i++) {
        if (strcmp(path, filtered_paths[i]) == 0)
            return 1;
    }
    
    return 0;
}

// Filter content to remove frida-related lines
static char* filter_content(char *content) {
    if (!content) return NULL;
    
    char *result = malloc(strlen(content) + 1);
    char *dst = result;
    char *line = strtok(content, "\n");
    
    while (line) {
        // Skip lines containing frida/fs-server/gum
        if (!strstr(line, "frida") && 
            !strstr(line, "fs-server") &&
            !strstr(line, "gum") &&
            !strstr(line, ":51234") &&  // Our custom port
            !strstr(line, "thread_name_obfuscator")) {
            dst += sprintf(dst, "%s\n", line);
        }
        line = strtok(NULL, "\n");
    }
    
    return result;
}

FILE* fopen(const char *path, const char *mode) {
    if (!real_fopen) init_hooks();
    
    if (should_filter(path)) {
        // TODO: Return filtered content
        // For now, pass through
    }
    
    return real_fopen(path, mode);
}

FILE* fopen64(const char *path, const char *mode) {
    if (!real_fopen64) init_hooks();
    
    if (should_filter(path)) {
        // TODO: Return filtered content
    }
    
    return real_fopen64(path, mode);
}
