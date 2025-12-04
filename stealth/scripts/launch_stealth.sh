#!/bin/bash
# Comprehensive Frida Stealth Launcher
# Combines all Phase 1-3 protections

set -e

DEVICE_PATH="/data/local/tmp"
SERVER_NAME="fs-server"

echo "=== Frida Stealth Launcher v2.0 ==="
echo "Deploying Phase 1-3 protections..."

# Upload all components
echo "[1/5] Uploading server and protection modules..."
adb push build/subprojects/frida-core/server/android_fs-server $DEVICE_PATH/$SERVER_NAME
adb push thread_name_obfuscator.so $DEVICE_PATH/
adb push proc_hider.so $DEVICE_PATH/
adb push antidebug_bypass.so $DEVICE_PATH/

# Set permissions
echo "[2/5] Setting permissions..."
adb shell "chmod 755 $DEVICE_PATH/$SERVER_NAME $DEVICE_PATH/*.so"

# Kill any existing instances
echo "[3/5] Cleaning up old instances..."
adb shell "killall $SERVER_NAME" 2>/dev/null || true

# Launch with all protections
echo "[4/5] Launching stealth server..."
adb shell "LD_PRELOAD='$DEVICE_PATH/thread_name_obfuscator.so:$DEVICE_PATH/proc_hider.so:$DEVICE_PATH/antidebug_bypass.so' nohup $DEVICE_PATH/$SERVER_NAME > /dev/null 2>&1 &"

sleep 3

# Verify
echo "[5/5] Verifying..."
PID=$(adb shell "pidof $SERVER_NAME")
if [ -z "$PID" ]; then
    echo "❌ Failed to start server"
    exit 1
fi

echo "✅ Stealth server running (PID: $PID)"
echo ""
echo "Active protections:"
echo "  ✓ Port obfuscation (51234)"
echo "  ✓ Thread name masking"
echo "  ✓ /proc filesystem hiding"
echo "  ✓ Anti-debugging bypass"
echo "  ✓ String encryption (runtime)"
echo ""
echo "Run 'python3 test_frida_detection.py' to verify"
