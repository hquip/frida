#!/bin/bash
# Create a single self-extracting executable for Ultimate Stealth Frida
# Usage: ./package_standalone.sh

set -e

OUTPUT_FILE="frida-server-stealth"
BUILD_DIR="build"
MODULES_DIR="$(pwd)/stealth/modules"
SERVER_BIN="$BUILD_DIR/release/fs-server-ultimate"

# Ensure ultimate build exists
if [ ! -f "$SERVER_BIN" ]; then
    echo "Building ultimate server first..."
    ./stealth/scripts/build_ultimate.sh
fi

echo "Creating single-file executable: $OUTPUT_FILE"

# Create the wrapper script header
cat > "$OUTPUT_FILE" <<'EOF'
#!/bin/sh
# Self-extracting Frida Stealth Server
# Dynamic protection module loading

export TMPDIR=/data/local/tmp
INSTALL_DIR=$(mktemp -d $TMPDIR/fs-stealth.XXXXXX)

# Extract payload
tail -n +$(($(grep -a -n "^__PAYLOAD_BELOW__$" $0 | head -n 1 | cut -d : -f 1) + 1)) $0 | tar xz -C $INSTALL_DIR

# Setup environment
chmod 755 $INSTALL_DIR/*

# Dynamically build LD_PRELOAD from all .so files in the install dir
PRELOAD_LIST=""
for lib in $INSTALL_DIR/*.so; do
    if [ -f "$lib" ]; then
        if [ -z "$PRELOAD_LIST" ]; then
            PRELOAD_LIST="$lib"
        else
            PRELOAD_LIST="$PRELOAD_LIST:$lib"
        fi
    fi
done

export LD_PRELOAD=$PRELOAD_LIST

# Run server
echo "🚀 Launching Stealth Frida Server..."
echo "    Loaded modules: $(echo $LD_PRELOAD | tr ':' '\n' | wc -l)"
$INSTALL_DIR/fs-server-ultimate "$@" &
PID=$!
echo "✅ Server running (PID: $PID)"

# Wait for server
wait $PID

# Cleanup on exit (trap)
rm -rf $INSTALL_DIR
exit 0

__PAYLOAD_BELOW__
EOF

# Dynamically find all .so files to include
echo "Adding modules from $MODULES_DIR:"
MODULE_FILES=""
for f in "$MODULES_DIR"/*.so; do
    if [ -f "$f" ]; then
        fname=$(basename "$f")
        echo "  + $fname"
        MODULE_FILES="$MODULE_FILES $fname"
    fi
done

# Create tarball of all components
# We change directory to MODULES_DIR to add .so files without path prefix
# And we use -C to jump to build dir for the server binary
tar czf - \
    -C "$BUILD_DIR/release" fs-server-ultimate \
    -C "$MODULES_DIR" $MODULE_FILES \
    >> "$OUTPUT_FILE"

chmod +x "$OUTPUT_FILE"

echo ""
echo "🎉 Success! Created '$OUTPUT_FILE'"
echo "Size: $(du -h $OUTPUT_FILE | cut -f1)"
echo ""
echo "Usage:"
echo "  1. Push to device: adb push $OUTPUT_FILE /data/local/tmp/"
echo "  2. Run: adb shell /data/local/tmp/$OUTPUT_FILE"
echo ""
