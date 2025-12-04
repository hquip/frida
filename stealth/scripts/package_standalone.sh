#!/bin/bash
# Packages the compiled server and modules into a self-extracting executable

# Default target
TARGET="android"
if [ "$1" == "ios" ]; then
    TARGET="ios"
fi

BUILD_DIR="build"
MODULES_DIR="$(pwd)/stealth/modules"
OUTPUT_FILE="frida-server-stealth"
SERVER_BIN="$BUILD_DIR/release/fs-server-ultimate"

if [ ! -f "$SERVER_BIN" ]; then
    echo "Error: Server binary not found at $SERVER_BIN"
    exit 1
fi

echo "Packaging for $TARGET..."

# Determine extensions and env vars
if [ "$TARGET" == "ios" ]; then
    EXT="dylib"
    PRELOAD_VAR="DYLD_INSERT_LIBRARIES"
else
    EXT="so"
    PRELOAD_VAR="LD_PRELOAD"
fi

# Create the wrapper script header
cat > header.sh <<EOF
#!/bin/bash
# Frida Stealth Launcher
# Auto-extracts and runs with protection modules

# Create a temporary directory
TMP_DIR=\$(mktemp -d /tmp/frida-stealth.XXXXXX)
# Extract payload to temp dir
tail -n +\$((\$(wc -l < "\$0") + 1)) "\$0" | tar xz -C "\$TMP_DIR"

# Set up environment
export FRIDA_SERVER_ADDRESS=127.0.0.1:27042

# Build the preload list dynamically
MODULE_LIST=""
for f in "\$TMP_DIR"/*.$EXT; do
    if [ -f "\$f" ]; then
        if [ -z "\$MODULE_LIST" ]; then
            MODULE_LIST="\$f"
        else
            MODULE_LIST="\$MODULE_LIST:\$f"
        fi
    fi
done

export $PRELOAD_VAR="\$MODULE_LIST"

# Launch the server
# exec "\$TMP_DIR/fs-server-ultimate" "\$@"
# Use exec -a to spoof process name if possible (linux only usually, but harmless on others?)
# For iOS/BSD, exec -a might not work or be needed.
exec "\$TMP_DIR/fs-server-ultimate" "\$@"

# Cleanup (this won't run after exec, but good for reference)
rm -rf "\$TMP_DIR"
EOF

chmod +x header.sh

# Start building the final binary
cp header.sh "$OUTPUT_FILE"

# Dynamically find all module files to include
echo "Adding modules from $MODULES_DIR:"
MODULE_FILES=""
for f in "$MODULES_DIR"/*.$EXT; do
    if [ -f "$f" ]; then
        fname=$(basename "$f")
        echo "  + $fname"
        MODULE_FILES="$MODULE_FILES $fname"
    fi
done

# Create tarball of all components
# Note: We use -C to change directory to avoid full paths in tar
# We need to add server and modules.
# To keep it simple, we'll copy everything to a temp dir and tar that.
PKG_TMP=$(mktemp -d)
cp "$SERVER_BIN" "$PKG_TMP/"
if [ -n "$MODULE_FILES" ]; then
    for mod in $MODULE_FILES; do
        cp "$MODULES_DIR/$mod" "$PKG_TMP/"
    done
fi

tar czf - -C "$PKG_TMP" . >> "$OUTPUT_FILE"

rm -rf "$PKG_TMP"
rm header.sh
chmod +x "$OUTPUT_FILE"

echo "🎉 Success! Created '$OUTPUT_FILE'"
echo "Size: $(du -h "$OUTPUT_FILE" | cut -f1)"

echo ""
echo "Usage:"
echo "  1. Push to device: adb push $OUTPUT_FILE /data/local/tmp/"
echo "  2. Run: adb shell /data/local/tmp/$OUTPUT_FILE"
echo ""
