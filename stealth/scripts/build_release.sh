#!/bin/bash
# Master Build Script for Frida Stealth
# Compiles EVERYTHING: Modules + Server + Package

set -e

export ANDROID_NDK_ROOT=/root/frida/android-ndk-r25c
export PATH=/root/frida/deps/toolchain-linux-x86_64/bin:$PATH
export NINJA=/root/frida/deps/toolchain-linux-x86_64/bin/ninja

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   Frida Stealth - 完整编译脚本                           ║"
echo "╚══════════════════════════════════════════════════════════╝"

# 1. Compile Protection Modules
echo ""
echo "[1/4] 编译 14 个保护模块..."
cd stealth/modules
make clean
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "❌ 模块编译失败！"
    exit 1
fi
echo "    ✓ 模块编译完成"
cd ../..

# 2. Compile Frida Server Core
echo ""
echo "[2/4] 编译 Frida Server (可能需要几分钟)..."
# Ensure environment is configured for Android ARM64
if [ ! -f "build/build.ninja" ]; then
    echo "    配置构建环境..."
    ./configure --host=android-arm64
fi

# Build everything, ignoring errors (to skip python bindings issues)
ninja -C build -k 0
# Check if binary exists
SERVER_BIN="build/subprojects/frida-core/server/android_fs-server"
if [ ! -f "$SERVER_BIN" ]; then
    # Try finding it in other locations just in case
    SERVER_BIN=$(find build -name "frida-server" -o -name "fs-server" | head -n 1)
fi

if [ ! -f "$SERVER_BIN" ]; then
    echo "❌ Server 编译失败！找不到输出文件。"
    exit 1
fi
echo "    ✓ Server 编译完成: $SERVER_BIN"

# 3. Optimize Server (Strip & Obfuscate)
echo ""
echo "[3/4] 优化 Server 二进制..."
# Copy to a clean location
mkdir -p build/release
cp "$SERVER_BIN" build/release/fs-server-ultimate
aarch64-linux-gnu-strip --strip-all build/release/fs-server-ultimate
echo "    ✓ 符号已清除"

# 4. Package Everything
echo ""
echo "[4/4] 打包单文件可执行程序..."
./stealth/scripts/package_standalone.sh

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ 全部编译完成！"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "输出文件: frida-server-stealth"
echo "大小: $(du -h frida-server-stealth | cut -f1)"
echo ""
echo "使用方法:"
echo "  adb push frida-server-stealth /data/local/tmp/"
echo "  adb shell /data/local/tmp/frida-server-stealth"
echo ""
