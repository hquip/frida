#define _GNU_SOURCE
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>

// Optimization #5: Memory Protection Enhancement

// Protected memory regions
typedef struct {
    void *addr;
    size_t size;
    int original_prot;
} ProtectedRegion;

#define MAX_PROTECTED_REGIONS 128
static ProtectedRegion protected_regions[MAX_PROTECTED_REGIONS];
static int num_protected = 0;

// Register a sensitive memory region for protection
static void register_protected_region(void *addr, size_t size) {
    if (num_protected >= MAX_PROTECTED_REGIONS) return;
    
    protected_regions[num_protected].addr = addr;
    protected_regions[num_protected].size = size;
    protected_regions[num_protected].original_prot = PROT_READ | PROT_WRITE;
    
    // Immediately protect it
    mprotect(addr, size, PROT_NONE);
    num_protected++;
}

// Temporarily unprotect for access
static void unprotect_for_access(void *addr) {
    for (int i = 0; i < num_protected; i++) {
        if (addr >= protected_regions[i].addr &&
            addr < protected_regions[i].addr + protected_regions[i].size) {
            mprotect(protected_regions[i].addr, 
                    protected_regions[i].size,
                    PROT_READ | PROT_WRITE);
            return;
        }
    }
}

// Re-protect after access
static void reprotect_after_access(void *addr) {
    for (int i = 0; i < num_protected; i++) {
        if (addr >= protected_regions[i].addr &&
            addr < protected_regions[i].addr + protected_regions[i].size) {
            mprotect(protected_regions[i].addr, 
                    protected_regions[i].size,
                    PROT_NONE);
            return;
        }
    }
}

// Secure malloc wrapper
void* secure_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr) {
        register_protected_region(ptr, size);
    }
    return ptr;
}

// Secure free wrapper
void secure_free(void *ptr) {
    // Unregister from protection
    for (int i = 0; i < num_protected; i++) {
        if (protected_regions[i].addr == ptr) {
            // Zero out memory before freeing
            memset(ptr, 0, protected_regions[i].size);
            mprotect(ptr, protected_regions[i].size, PROT_READ | PROT_WRITE);
            
            // Remove from list
            for (int j = i; j < num_protected - 1; j++) {
                protected_regions[j] = protected_regions[j + 1];
            }
            num_protected--;
            break;
        }
    }
    
    free(ptr);
}
