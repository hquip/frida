#!/usr/bin/env python3
"""
Runtime String Encryptor for Frida
Encrypts all frida-related strings in the binary at build time
"""

import sys
import struct
import random

def xor_encrypt(data, key):
    """XOR encryption for string obfuscation"""
    return bytes([b ^ (key + i) % 256 for i, b in enumerate(data)])

def generate_decryptor_stub():
    """Generate C code for runtime string decryption"""
    return """
#ifndef STRING_OBFUSCATOR_H
#define STRING_OBFUSCATOR_H

#include <stdint.h>
#include <string.h>

static inline void deobf_str(char *str, size_t len, uint8_t key) {
    for (size_t i = 0; i < len; i++) {
        str[i] ^= (key + i) % 256;
    }
}

// Auto-deobfuscating string macro
#define OBFSTR(str, key) \\
    ({ \\
        static char buf[] = str; \\
        static int init = 0; \\
        if (!init) { \\
            deobf_str(buf, sizeof(buf)-1, key); \\
            init = 1; \\
        } \\
        buf; \\
    })

#endif
"""

def process_binary(input_path, output_path):
    """Process binary to encrypt sensitive strings"""
    with open(input_path, 'rb') as f:
        data = bytearray(f.read())
    
    # Patterns to encrypt
    patterns = [
        b'frida',
        b'FRIDA',
        b'Frida',
        b'gum',
        b'GUM',
        b're.frida',
        b'LIBFRIDA'
    ]
    
    key = random.randint(1, 255)
    
    for pattern in patterns:
        offset = 0
        while True:
            offset = data.find(pattern, offset)
            if offset == -1:
                break
            # Encrypt in place
            encrypted = xor_encrypt(pattern, key)
            data[offset:offset+len(pattern)] = encrypted
            offset += len(pattern)
    
    with open(output_path, 'wb') as f:
        f.write(data)
    
    return key

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: string_obfuscator.py <input_binary> <output_binary>")
        sys.exit(1)
    
    key = process_binary(sys.argv[1], sys.argv[2])
    
    # Save decryption key for stub injection
    with open(sys.argv[2] + '.key', 'w') as f:
        f.write(str(key))
    
    print(f"String obfuscation complete. Key: {key}")
