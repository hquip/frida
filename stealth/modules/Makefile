# Makefile for Frida Stealth Framework
# Builds all protection modules

CC = aarch64-linux-gnu-gcc
CFLAGS = -shared -fPIC -O2 -Wall
SRC_DIR = src
OUT_DIR = .

MODULES = env_cleaner.so \
          thread_name_obfuscator.so \
          proc_hider.so \
          antidebug_bypass.so \
          behavior_randomizer.so \
          traffic_obfuscator.so \
          sandbox_bypass.so \
          memory_protector.so \
          hook_detector.so \
          rdtsc_virtualizer.so \
          chacha20_tls.so \
          selinux_spoofer.so \
          art_hook_hider.so \
          hardware_spoofer.so

.PHONY: all clean install

all: $(MODULES)

%.so: $(SRC_DIR)/%.c
	@echo "Building $@..."
	$(CC) $(CFLAGS) -o $(OUT_DIR)/$@ $<

clean:
	rm -f $(OUT_DIR)/*.so

install:
	adb push $(OUT_DIR)/*.so /data/local/tmp/
	@echo "All modules installed to /data/local/tmp/"

list:
	@echo "Available modules:"
	@ls -lh $(OUT_DIR)/*.so 2>/dev/null || echo "No modules built yet. Run 'make' first."

help:
	@echo "Frida Stealth Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make          - Build all modules"
	@echo "  make clean    - Remove all .so files"
	@echo "  make install  - Push to Android device via ADB"
	@echo "  make list     - List built modules"
	@echo "  make help     - Show this help"
