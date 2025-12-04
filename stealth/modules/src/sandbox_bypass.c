#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>

// Phase 3.3: Sandbox/Emulator Detection Bypass

// Common sandbox/emulator indicators
static int is_sandbox_check(const char *path) {
    if (!path) return 0;
    
    const char *sandbox_indicators[] = {
        "/system/bin/qemud",          // QEMU
        "/dev/socket/qemud",
        "/system/lib/libc_malloc_debug_qemu.so",
        "/sys/qemu_trace",
        "/system/bin/microvirt",      // Android emulator
        "/dev/goldfish",
        "/proc/tty/drivers",          // Check for goldfish
        "/sys/devices/virtual/input", // Virtual input devices
        NULL
    };
    
    for (int i = 0; sandbox_indicators[i]; i++) {
        if (strstr(path, sandbox_indicators[i]))
            return 1;
    }
    
    return 0;
}

// Hook stat to hide sandbox indicators
int stat(const char *pathname, struct stat *statbuf) {
    static int (*real_stat)(const char*, struct stat*) = NULL;
    if (!real_stat) {
        real_stat = dlsym(RTLD_NEXT, "stat");
    }
    
    // Return ENOENT for sandbox indicator files
    if (is_sandbox_check(pathname)) {
        return -1; // File not found
    }
    
    return real_stat(pathname, statbuf);
}

// Hook access
int access(const char *pathname, int mode) {
    static int (*real_access)(const char*, int) = NULL;
    if (!real_access) {
        real_access = dlsym(RTLD_NEXT, "access");
    }
    
    if (is_sandbox_check(pathname)) {
        return -1;
    }
    
    return real_access(pathname, mode);
}

// Fake device properties to appear like real device
FILE* fopen(const char *pathname, const char *mode) {
    static FILE* (*real_fopen)(const char*, const char*) = NULL;
    if (!real_fopen) {
        real_fopen = dlsym(RTLD_NEXT, "fopen");
    }
    
    // Intercept build.prop reads to fake device info
    if (pathname && strstr(pathname, "build.prop")) {
        // Could return fake build.prop here
    }
    
    if (is_sandbox_check(pathname)) {
        return NULL;
    }
    
    return real_fopen(pathname, mode);
}
