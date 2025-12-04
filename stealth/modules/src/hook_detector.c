#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

// Optimization #6: Hook Detection & Direct Syscall fallback

// Check if a function pointer has been hooked
static int is_function_hooked(void *func_ptr) {
    if (!func_ptr) return 0;
    
    // Check for common hook patterns
    unsigned char *code = (unsigned char *)func_ptr;
    
    // Check for jump instructions (common hook method)
    if (code[0] == 0xE9 ||  // JMP rel32 (x86)
        code[0] == 0xEB ||  // JMP rel8
        code[0] == 0xFF) {  // JMP indirect
        return 1;
    }
    
    // Check for ARM64 branch instructions
    uint32_t *arm_code = (uint32_t *)func_ptr;
    uint32_t instr = arm_code[0];
    
    // B instruction: 000101xx xxxxxxxx xxxxxxxx xxxxxxxx
    if ((instr & 0xFC000000) == 0x14000000) {
        return 1;
    }
    
    // BL instruction: 100101xx xxxxxxxx xxxxxxxx xxxxxxxx
    if ((instr & 0xFC000000) == 0x94000000) {
        return 1;
    }
    
    return 0;
}

// Direct syscall wrappers (bypass libc hooks)
static long direct_syscall_3(long number, long arg1, long arg2, long arg3) {
    register long x8 __asm__("x8") = number;
    register long x0 __asm__("x0") = arg1;
    register long x1 __asm__("x1") = arg2;
    register long x2 __asm__("x2") = arg3;
    
    __asm__ __volatile__(
        "svc #0"
        : "+r"(x0)
        : "r"(x8), "r"(x1), "r"(x2)
        : "memory"
    );
    
    return x0;
}

// Safe open using direct syscall if libc is hooked
int safe_open(const char *pathname, int flags, mode_t mode) {
    static int (*real_open)(const char*, int, mode_t) = NULL;
    
    if (!real_open) {
        real_open = dlsym(RTLD_NEXT, "open");
    }
    
    // Check if open is hooked
    if (is_function_hooked(real_open)) {
        // Use direct syscall
        return (int)direct_syscall_3(__NR_openat, AT_FDCWD, 
                                     (long)pathname, flags);
    }
    
    return real_open(pathname, flags, mode);
}

// Safe read using direct syscall if hooked
ssize_t safe_read(int fd, void *buf, size_t count) {
    static ssize_t (*real_read)(int, void*, size_t) = NULL;
    
    if (!real_read) {
        real_read = dlsym(RTLD_NEXT, "read");
    }
    
    if (is_function_hooked(real_read)) {
        return direct_syscall_3(__NR_read, fd, (long)buf, count);
    }
    
    return real_read(fd, buf, count);
}
