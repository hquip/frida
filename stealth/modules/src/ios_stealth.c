#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

// iOS Stealth Module
// 1. Cleans DYLD_INSERT_LIBRARIES
// 2. Basic Anti-Jailbreak / Anti-Debug hiding

// Constructor to run at load time
static void __attribute__((constructor)) init_ios_stealth() {
    // 1. Clean environment
    unsetenv("DYLD_INSERT_LIBRARIES");
    unsetenv("_MSSafeMode"); // Substrate safe mode
}

// Hook getenv to hide DYLD variables
char* getenv(const char *name) {
    static char* (*real_getenv)(const char*) = NULL;
    if (!real_getenv) {
        real_getenv = dlsym(RTLD_NEXT, "getenv");
    }
    
    if (name && (strcmp(name, "DYLD_INSERT_LIBRARIES") == 0 ||
                 strcmp(name, "_MSSafeMode") == 0)) {
        return NULL;
    }
    
    return real_getenv(name);
}

// Hook sysctl to hide process info (e.g. tracer)
int sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen) {
    static int (*real_sysctl)(int *, u_int, void *, size_t *, void *, size_t) = NULL;
    if (!real_sysctl) {
        real_sysctl = dlsym(RTLD_NEXT, "sysctl");
    }

    int result = real_sysctl(name, namelen, oldp, oldlenp, newp, newlen);

    // Check for KERN_PROC_PID (process info)
    if (name[0] == CTL_KERN && name[1] == KERN_PROC && name[2] == KERN_PROC_PID) {
        if (oldp && result == 0) {
            struct kinfo_proc *info = (struct kinfo_proc *)oldp;
            // Hide tracer (P_TRACED flag)
            if ((info->kp_proc.p_flag & P_TRACED) != 0) {
                info->kp_proc.p_flag &= ~P_TRACED;
            }
        }
    }

    return result;
}
