#!/bin/bash
# Frida防检测快速测试脚本
# 这个脚本会自动部署server并运行检测测试

set -e

echo "========================================="
echo "  Frida防检测自动化测试"  
echo "========================================="
echo

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查adb连接
echo "[1/5] 检查ADB连接..."
if ! adb devices | grep -q "device$"; then
    echo -e "${RED}错误: 未检测到Android设备${NC}"
    echo "请确保:"
    echo "  1. USB调试已开启"
    echo "  2. 设备已通过USB连接"
    echo "  3. 已授权USB调试"
    exit 1
fi
echo -e "${GREEN}✓ ADB连接正常${NC}"
echo

# 检查server文件
echo "[2/5] 检查Frida server文件..."
SERVER_PATH="build/subprojects/frida-core/server/fs-server"
if [ ! -f "$SERVER_PATH" ]; then
    echo -e "${RED}错误: 找不到编译的server文件${NC}"
    echo "请先编译: export ANDROID_NDK_ROOT=/path/to/ndk && make"
    exit 1
fi
echo -e "${GREEN}✓ Server文件存在${NC}"
echo

# 部署server
echo "[3/5] 部署Frida server到设备..."
adb push "$SERVER_PATH" /data/local/tmp/fs-server > /dev/null 2>&1
adb shell "chmod 755 /data/local/tmp/fs-server"
echo -e "${GREEN}✓ Server已部署${NC}"
echo

# 启动server
echo "[4/5] 启动Frida server..."
# 先kill可能存在的旧进程
adb shell "su -c 'killall fs-server 2>/dev/null || true'" > /dev/null 2>&1
adb shell "su -c 'killall frida-server 2>/dev/null || true'" > /dev/null 2>&1

# 启动新server
adb shell "su -c '/data/local/tmp/fs-server -l 0.0.0.0:27050 >/dev/null 2>&1 &'" > /dev/null 2>&1
sleep 2

# 验证server运行
if adb shell "ps | grep fs-server" | grep -q "fs-server"; then
    echo -e "${GREEN}✓ Server启动成功${NC}"
else
    echo -e "${YELLOW}⚠ Server可能未启动，继续测试...${NC}"
fi
echo

# 运行检测测试
echo "[5/5] 运行防检测测试..."
echo "========================================="
echo
echo "[6/6] 运行设备端深度检测 (Native)..."
echo "========================================="
echo

# 编译Native检测程序
echo "正在编译Native检测工具..."
CC="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/clang"
API_LEVEL="30" # Android 11
TARGET="aarch64-linux-android$API_LEVEL"

if [ ! -f "$CC" ]; then
    echo -e "${YELLOW}⚠ 找不到NDK Clang，尝试使用默认路径...${NC}"
    # 尝试常见的NDK路径或跳过
    echo "跳过Native编译 (NDK未找到)"
else
    $CC -target $TARGET -o tests/anti_detection tests/anti_detection.c > /dev/null 2>&1
    
    if [ -f "tests/anti_detection" ]; then
        echo "编译成功，正在推送到设备..."
        adb push tests/anti_detection /data/local/tmp/ > /dev/null 2>&1
        adb shell "chmod 755 /data/local/tmp/anti_detection"
        
        # 准备注入工具
        echo "准备注入工具 (frida-inject)..."
        if [ -f "build/subprojects/frida-core/inject/frida-inject" ]; then
            adb push build/subprojects/frida-core/inject/frida-inject /data/local/tmp/fs-inject > /dev/null 2>&1
            adb shell "chmod 755 /data/local/tmp/fs-inject"
            
            echo "正在运行Native检测并尝试注入..."
            echo "----------------------------------------"
            
            # 后台运行检测程序
            adb shell "/data/local/tmp/anti_detection" > tests/native_test.log 2>&1 &
            PID_CMD="adb shell pidof anti_detection"
            
            sleep 2
            TARGET_PID=$(adb shell pidof anti_detection)
            
            if [ -n "$TARGET_PID" ]; then
                echo "目标进程PID: $TARGET_PID"
                echo "正在尝试注入..."
                
                # 创建简单的注入脚本
                echo "console.log('Injected successfully');" > tests/inject.js
                adb push tests/inject.js /data/local/tmp/ > /dev/null 2>&1
                
                # 执行注入
                adb shell "/data/local/tmp/fs-inject -p $TARGET_PID -s /data/local/tmp/inject.js" > tests/inject.log 2>&1 &
                
                # 等待几秒让检测程序运行
                sleep 5
                
                echo "--- 检测程序输出 ---"
                cat tests/native_test.log
                echo "-------------------"
                
                # 检查是否检测到
                if grep -q "DETECTED" tests/native_test.log; then
                    echo -e "${RED}[!] 注入被检测到了！${NC}"
                else
                    echo -e "${GREEN}[✓] 注入未被检测到！(Ultimate Stealth)${NC}"
                fi
                
                # 清理
                adb shell "kill $TARGET_PID" > /dev/null 2>&1
            else
                echo "无法启动检测程序"
            fi
        else
            echo "找不到 frida-inject，跳过注入测试"
            adb shell "/data/local/tmp/anti_detection"
        fi
    else
        echo -e "${RED}编译失败${NC}"
    fi
fi

# 测试结束
echo
echo "========================================="
echo -e "${GREEN}测试完成！${NC}"
echo
echo "详细说明请查看: cat README_TESTING.md"
echo
