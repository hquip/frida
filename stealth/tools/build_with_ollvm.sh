#!/bin/bash
# LLVM Obfuscation Build Script
# Uses Obfuscator-LLVM (ollvm) to obfuscate frida-server

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   LLVM Obfuscation Build System                          ║"
echo "║   Code Obfuscation for Anti-Reverse Engineering          ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Check if ollvm is available
if ! command -v obfuscator &> /dev/null; then
    echo "❌ Obfuscator-LLVM not found!"
    echo ""
    echo "Installation instructions:"
    echo "  1. Download: git clone https://github.com/obfuscator-llvm/obfuscator"
    echo "  2. Build: cd obfuscator && mkdir build && cd build"
    echo "  3. Configure: cmake -DCMAKE_BUILD_TYPE=Release .."
    echo "  4. Compile: make -j\$(nproc)"
    echo "  5. Install: sudo make install"
    echo ""
    echo "For quick start, you can use Docker:"
    echo "  docker pull cryptax/obfuscator-llvm"
    echo ""
    exit 1
fi

echo "[1/5] Configuring build with LLVM obfuscation..."

# LLVM obfuscation flags
OBFUSCATION_FLAGS=(
    "-mllvm -fla"           # Control Flow Flattening
    "-mllvm -sub"           # Instruction Substitution
    "-mllvm -bcf"           # Bogus Control Flow
    "-mllvm -sobf"          # String Obfuscation
)

# Export compiler settings
export CC="clang"
export CXX="clang++"
export CFLAGS="${OBFUSCATION_FLAGS[@]} -O2"
export CXXFLAGS="${OBFUSCATION_FLAGS[@]} -O2"

echo "    Obfuscation enabled:"
echo "      ✓ Control Flow Flattening (-fla)"
echo "      ✓ Instruction Substitution (-sub)"
echo "      ✓ Bogus Control Flow (-bcf)"
echo "      ✓ String Obfuscation (-sobf)"
echo ""

echo "[2/5] Cleaning previous build..."
cd "$PROJECT_ROOT"
rm -rf build_obfuscated
mkdir -p build_obfuscated

echo "[3/5] Configuring frida with obfuscation..."
./configure \
    --host=android-arm64 \
    --prefix="$PROJECT_ROOT/build_obfuscated" \
    --enable-static \
    --disable-shared

echo "[4/5] Building with obfuscation (this may take 30-60 minutes)..."
make -j$(nproc) VERBOSE=1

echo "[5/5] Post-processing..."
OBFUSCATED_SERVER="build_obfuscated/frida-core/server/android_fs-server"

if [ -f "$OBFUSCATED_SERVER" ]; then
    # Strip symbols
    aarch64-linux-gnu-strip --strip-all "$OBFUSCATED_SERVER"
    
    # Show results
    SIZE=$(stat -c%s "$OBFUSCATED_SERVER")
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "✅ LLVM Obfuscated Build Complete!"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "Output: $OBFUSCATED_SERVER"
    echo "Size:   $(numfmt --to=iec-i --suffix=B $SIZE)"
    echo ""
    echo "Obfuscation Applied:"
    echo "  ✓ Control flow flattened (anti-CFG analysis)"
    echo "  ✓ Instructions substituted (anti-pattern matching)"
    echo "  ✓ Bogus branches inserted (confuse disassembly)"
    echo "  ✓ Strings obfuscated (hide constants)"
    echo ""
    echo "Next Steps:"
    echo "  1. Test: file $OBFUSCATED_SERVER"
    echo "  2. Deploy: adb push $OBFUSCATED_SERVER /data/local/tmp/"
    echo "  3. Verify: IDA Pro / Ghidra analysis should be much harder"
    echo ""
else
    echo "❌ Build failed! Check logs above."
    exit 1
fi
