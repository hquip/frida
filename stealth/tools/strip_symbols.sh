#!/bin/bash
# Critical Optimization #2: Strip all symbols and debug info

set -e

INPUT="$1"
OUTPUT="$2"

if [ -z "$INPUT" ] || [ -z "$OUTPUT" ]; then
    echo "Usage: $0 <input_binary> <output_binary>"
    exit 1
fi

echo "[1/4] Copying binary..."
cp "$INPUT" "$OUTPUT"

echo "[2/4] Stripping all symbols..."
aarch64-linux-gnu-strip --strip-all "$OUTPUT"

echo "[3/4] Removing debug sections..."
aarch64-linux-gnu-strip --remove-section=.comment \
                        --remove-section=.note \
                        --remove-section=.note.ABI-tag \
                        --remove-section=.note.gnu.build-id \
                        --remove-section=.gnu.version \
                        "$OUTPUT"

echo "[4/4] Final cleanup..."
# Remove section headers (requires sstrip if available)
if command -v sstrip &> /dev/null; then
    sstrip "$OUTPUT"
    echo "    ✓ Section headers removed"
else
    echo "    ⚠ sstrip not found, skipping section header removal"
fi

BEFORE=$(stat -c%s "$INPUT")
AFTER=$(stat -c%s "$OUTPUT")
SAVED=$((BEFORE - AFTER))

echo ""
echo "Symbol Stripping Complete:"
echo "  Before: $(numfmt --to=iec-i --suffix=B $BEFORE)"
echo "  After:  $(numfmt --to=iec-i --suffix=B $AFTER)"
echo "  Saved:  $(numfmt --to=iec-i --suffix=B $SAVED)"
echo ""
