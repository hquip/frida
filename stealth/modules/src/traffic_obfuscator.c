#define _GNU_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>

// Phase 3.2: Network Traffic Obfuscation

// Simple XOR cipher for traffic obfuscation
static void xor_cipher(unsigned char *data, size_t len, unsigned char key) {
    for (size_t i = 0; i < len; i++) {
        data[i] ^= (key + i) % 256;
    }
}

// Hook send() to obfuscate outgoing data
ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    static ssize_t (*real_send)(int, const void*, size_t, int) = NULL;
    if (!real_send) {
        real_send = dlsym(RTLD_NEXT, "send");
    }
    
    // Check if this looks like frida protocol traffic (heuristic)
    if (len > 10 && len < 65536) {
        unsigned char *obfuscated = malloc(len);
        if (obfuscated) {
            memcpy(obfuscated, buf, len);
            // Simple XOR obfuscation
            xor_cipher(obfuscated, len, 0x42);
            ssize_t result = real_send(sockfd, obfuscated, len, flags);
            free(obfuscated);
            return result;
        }
    }
    
    return real_send(sockfd, buf, len, flags);
}

// Hook recv() to deobfuscate incoming data
ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    static ssize_t (*real_recv)(int, void*, size_t, int) = NULL;
    if (!real_recv) {
        real_recv = dlsym(RTLD_NEXT, "recv");
    }
    
    ssize_t result = real_recv(sockfd, buf, len, flags);
    
    // Deobfuscate received data
    if (result > 0) {
        xor_cipher((unsigned char*)buf, result, 0x42);
    }
    
    return result;
}

// Hide connection to non-standard port
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    static int (*real_connect)(int, const struct sockaddr*, socklen_t) = NULL;
    if (!real_connect) {
        real_connect = dlsym(RTLD_NEXT, "connect");
    }
    
    // If connecting to our custom port (51234), mask it
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr_in = (struct sockaddr_in*)addr;
        if (ntohs(addr_in->sin_port) == 51234) {
            // Connection metadata obfuscation could go here
        }
    }
    
    return real_connect(sockfd, addr, addrlen);
}
