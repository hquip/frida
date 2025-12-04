#!/bin/bash
set -e

echo "========================================="
echo "  Frida 功能完整性验证"
echo "========================================="

# 1. 准备环境
echo "[1/4] 准备测试环境..."

# 检查设备状态
ADB_STATUS=$(adb devices | grep -w "device")
if [ -z "$ADB_STATUS" ]; then
    echo "错误: 未找到已连接的设备或设备未授权"
    echo "请检查:"
    echo "  1. USB连接是否正常"
    echo "  2. 手机屏幕上是否弹出USB调试授权框 (请点击允许)"
    adb devices
    exit 1
fi

adb push tests/functional_test.js /data/local/tmp/ > /dev/null 2>&1
adb push tests/anti_detection /data/local/tmp/ > /dev/null 2>&1
adb shell "chmod 755 /data/local/tmp/anti_detection"

if [ ! -f "build/subprojects/frida-core/inject/frida-inject" ]; then
    echo "错误: 找不到 fs-inject (frida-inject)"
    exit 1
fi
adb push build/subprojects/frida-core/inject/frida-inject /data/local/tmp/fs-inject > /dev/null 2>&1
adb shell "chmod 755 /data/local/tmp/fs-inject"

# 推送本地编译的服务端
# 推送本地编译的服务端
if [ ! -f "frida-server-stealth" ]; then
    echo "错误: 找不到 frida-server-stealth"
    exit 1
fi
echo "推送本地编译的 frida-server..."
adb push frida-server-stealth /data/local/tmp/fs-server > /dev/null 2>&1
adb shell "chmod 755 /data/local/tmp/fs-server"

# 启动 frida-server (后台运行)
echo "启动 frida-server..."
adb shell "killall fs-server" > /dev/null 2>&1 || true
adb shell "nohup /data/local/tmp/fs-server > /dev/null 2>&1 &"
sleep 3

# 验证 server 是否启动
SERVER_PID=$(adb shell "pidof fs-server")
if [ -z "$SERVER_PID" ]; then
    echo "警告: frida-server 未启动成功，但继续测试..."
else
    echo "frida-server 已启动 (PID: $SERVER_PID)"
fi


# 2. 启动目标进程
echo "[2/4] 启动目标进程..."
adb shell "nohup /data/local/tmp/anti_detection > /dev/null 2>&1 &"
# 等待进程启动
sleep 2
TARGET_PID=$(adb shell pidof anti_detection)

if [ -z "$TARGET_PID" ]; then
    echo "错误: 目标进程启动失败"
    exit 1
fi
echo "目标进程PID: $TARGET_PID"

# 3. 执行功能测试
echo "[3/4] 注入并运行功能测试..."
echo "----------------------------------------"

# 先尝试简单注入验证
echo "console.log('Inject check passed');" > tests/simple_check.js
adb push tests/simple_check.js /data/local/tmp/ > /dev/null 2>&1
adb shell "/data/local/tmp/fs-inject -p $TARGET_PID -s /data/local/tmp/simple_check.js -e" > /dev/null 2>&1
sleep 1

# 运行完整测试，使用 -e (eternalize) 并将输出重定向到文件
echo "正在运行测试..."
adb shell "/data/local/tmp/fs-inject -p $TARGET_PID -s /data/local/tmp/functional_test.js -e > /data/local/tmp/test_output.log 2>&1 &"

echo "等待测试脚本执行..."
sleep 5

echo "--- Test Output ---"
adb shell "cat /data/local/tmp/test_output.log"
echo "-------------------"

# 4. 清理
echo "[4/4] 清理环境..."
adb shell "kill $TARGET_PID" > /dev/null 2>&1

echo
echo "测试完成"
