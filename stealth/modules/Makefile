# Makefile for Frida Stealth Framework
# Builds all protection modules

# Default target is android
TARGET ?= android

ifeq ($(TARGET),ios)
    CC = xcrun --sdk iphoneos clang
    CFLAGS = -dynamiclib -arch arm64 -O2 -Wall -framework Foundation
    EXT = dylib
    OUT_DIR = .
else
    CC = aarch64-linux-gnu-gcc
    CFLAGS = -shared -fPIC -O2 -Wall
    EXT = so
    OUT_DIR = .
endif

SRC_DIR = src

MODULES = env_cleaner.$(EXT) \
          thread_name_obfuscator.$(EXT) \
          proc_hider.$(EXT) \
          antidebug_bypass.$(EXT) \
          behavior_randomizer.$(EXT) \
          traffic_obfuscator.$(EXT) \
          sandbox_bypass.$(EXT) \
          memory_protector.$(EXT) \
          hook_detector.$(EXT) \
          rdtsc_virtualizer.$(EXT) \
          chacha20_tls.$(EXT) \
          selinux_spoofer.$(EXT) \
          art_hook_hider.$(EXT) \
          hardware_spoofer.$(EXT)

.PHONY: all clean install list help

all: $(MODULES)

%.$(EXT): $(SRC_DIR)/%.c
	@echo "Building $@ for $(TARGET)..."
	$(CC) $(CFLAGS) -o $(OUT_DIR)/$@ $<

clean:
	rm -f $(OUT_DIR)/*.so $(OUT_DIR)/*.dylib

install:
	@if [ "$(TARGET)" = "android" ]; then \
		adb push $(OUT_DIR)/*.so /data/local/tmp/; \
		echo "All modules installed to /data/local/tmp/"; \
	else \
		echo "Install not supported for iOS via Makefile yet."; \
	fi

list:
	@echo "Available modules:"
	@ls -lh $(OUT_DIR)/*.$(EXT) 2>/dev/null || echo "No modules built yet. Run 'make' first."

help:
	@echo "Frida Stealth Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make TARGET=android   - Build for Android (default)"
	@echo "  make TARGET=ios       - Build for iOS"
	@echo "  make clean            - Remove all built files"
	@echo "  make install          - Push to device (Android only)"

