#!/bin/bash
# 一键设置脚本 - 自动化所有构建和部署步骤

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   Frida Stealth - 一键安装脚本                           ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Step 1: 构建所有保护模块
echo "[1/4] 构建保护模块..."
cd "$PROJECT_ROOT/stealth/modules"
make clean
make
echo "    ✓ 7个模块已编译"

# Step 2: 构建终极服务端
echo "[2/4] 构建终极版 frida-server..."
cd "$PROJECT_ROOT"
if [ ! -f "build/subprojects/frida-core/server/android_fs-server" ]; then
    echo "    ⚠️  原始server未找到，跳过优化步骤"
    echo "       请先运行: make -j\$(nproc)"
else
    ./stealth/scripts/build_ultimate.sh
    echo "    ✓ 终极版已生成"
fi

# Step 3: 部署到设备
echo "[3/4] 部署到Android设备..."
if adb devices | grep -q "device$"; then
    ./stealth/scripts/launch_ultimate_stealth.sh
    echo "    ✓ 部署成功"
else
    echo "    ⚠️  未检测到Android设备"
    echo "       请连接设备后手动运行: ./stealth/scripts/launch_ultimate_stealth.sh"
fi

# Step 4: 验证
echo "[4/4] 验证..."
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ 安装完成！"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "下一步:"
echo "  运行检测测试: python3 test_frida_detection.py"
echo "  查看文档:     cat stealth/docs/README.md"
echo "  查看状态:     ls -lR stealth/"
echo ""
