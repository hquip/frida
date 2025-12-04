#include <pthread.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Advanced thread name obfuscation
// Generates random innocent-looking thread names

static const char* innocent_names[] = {
    "kworker",
    "ksoftirqd",
    "migration",
    "watchdog",
    "cpuhp",
    "netns",
    "kcompactd",
    "khugepaged",
    "kswapd",
    "jbd2",
    "ext4-rsv-conv",
    "systemd-udevd",
    "systemd-journal",
    "dbus-daemon",
    "NetworkManager",
    "wpa_supplicant"
};

static char obfuscated_name[16];
static int name_initialized = 0;

__attribute__((constructor))
static void init_thread_name_obfuscation(void) {
    srand(time(NULL) ^ getpid());
    int idx = rand() % (sizeof(innocent_names) / sizeof(innocent_names[0]));
    snprintf(obfuscated_name, sizeof(obfuscated_name), "%s/%d", 
             innocent_names[idx], rand() % 10);
    name_initialized = 1;
}

// Hook for pthread_setname_np
int pthread_setname_np(pthread_t thread, const char *name) {
    // Intercept gmain and other frida-related names
    if (name && (strstr(name, "gmain") || strstr(name, "frida") || 
                 strstr(name, "gum") || strstr(name, "agent"))) {
        if (!name_initialized)
            init_thread_name_obfuscation();
        
        // Use obfuscated name instead
        return prctl(PR_SET_NAME, obfuscated_name, 0, 0, 0);
    }
    
    // Pass through other names
    return prctl(PR_SET_NAME, name, 0, 0, 0);
}
