#define _GNU_SOURCE
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdint.h>

// Optimization #8: TLS Encryption Upgrade (ChaCha20)

// Simplified ChaCha20 implementation
#define ROTL32(v,n) (((v) << (n)) | ((v) >> (32 - (n))))
#define U32TO8_LITTLE(p, v) \
    { (p)[0] = (v >>  0) & 0xff; (p)[1] = (v >>  8) & 0xff; \
      (p)[2] = (v >> 16) & 0xff; (p)[3] = (v >> 24) & 0xff; }

static void chacha20_quarter_round(uint32_t *state, int a, int b, int c, int d) {
    state[a] += state[b]; state[d] ^= state[a]; state[d] = ROTL32(state[d], 16);
    state[c] += state[d]; state[b] ^= state[c]; state[b] = ROTL32(state[b], 12);
    state[a] += state[b]; state[d] ^= state[a]; state[d] = ROTL32(state[d], 8);
    state[c] += state[d]; state[b] ^= state[c]; state[b] = ROTL32(state[b], 7);
}

static void chacha20_block(uint32_t *output, const uint32_t *input) {
    uint32_t state[16];
    memcpy(state, input, 64);
    
    for (int i = 0; i < 10; i++) {
        chacha20_quarter_round(state, 0, 4, 8, 12);
        chacha20_quarter_round(state, 1, 5, 9, 13);
        chacha20_quarter_round(state, 2, 6, 10, 14);
        chacha20_quarter_round(state, 3, 7, 11, 15);
        chacha20_quarter_round(state, 0, 5, 10, 15);
        chacha20_quarter_round(state, 1, 6, 11, 12);
        chacha20_quarter_round(state, 2, 7, 8, 13);
        chacha20_quarter_round(state, 3, 4, 9, 14);
    }
    
    for (int i = 0; i < 16; i++) {
        output[i] = state[i] + input[i];
    }
}

static void chacha20_encrypt(unsigned char *data, size_t len, const unsigned char *key) {
    uint32_t state[16] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, // "expand 32-byte k"
        0, 0, 0, 0, 0, 0, 0, 0,  // key (placeholder)
        0, 0, 0, 0  // counter + nonce
    };
    
    // Simplified: just use key to initialize state
    memcpy(&state[4], key, 32);
    
    uint32_t keystream[16];
    unsigned char *ks_bytes = (unsigned char *)keystream;
    
    for (size_t i = 0; i < len; i += 64) {
        chacha20_block(keystream, state);
        state[12]++; // Increment counter
        
        size_t chunk = (len - i < 64) ? (len - i) : 64;
        for (size_t j = 0; j < chunk; j++) {
            data[i + j] ^= ks_bytes[j];
        }
    }
}

// Static key (should be negotiated in production)
static unsigned char chacha_key[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

// Upgraded send with ChaCha20
ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    static ssize_t (*real_send)(int, const void*, size_t, int) = NULL;
    if (!real_send) {
        real_send = dlsym(RTLD_NEXT, "send");
    }
    
    if (len > 10 && len < 65536) {
        unsigned char *encrypted = malloc(len);
        if (encrypted) {
            memcpy(encrypted, buf, len);
            chacha20_encrypt(encrypted, len, chacha_key);
            ssize_t result = real_send(sockfd, encrypted, len, flags);
            free(encrypted);
            return result;
        }
    }
    
    return real_send(sockfd, buf, len, flags);
}

// Upgraded recv with ChaCha20
ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    static ssize_t (*real_recv)(int, void*, size_t, int) = NULL;
    if (!real_recv) {
        real_recv = dlsym(RTLD_NEXT, "recv");
    }
    
    ssize_t result = real_recv(sockfd, buf, len, flags);
    
    if (result > 0) {
        chacha20_encrypt((unsigned char*)buf, result, chacha_key);
    }
    
    return result;
}
