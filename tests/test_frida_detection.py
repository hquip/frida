#!/usr/bin/env python3
"""
Frida反检测测试脚本
用于验证编译的Frida server是否能绕过常见的检测方法
"""

import subprocess
import sys
import re

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    END = '\033[0m'
    BOLD = '\033[1m'

def print_header(text):
    print(f"\n{Colors.BOLD}{Colors.BLUE}{'='*60}{Colors.END}")
    print(f"{Colors.BOLD}{Colors.BLUE}{text:^60}{Colors.END}")
    print(f"{Colors.BOLD}{Colors.BLUE}{'='*60}{Colors.END}\n")

def print_test(name, passed, details=""):
    status = f"{Colors.GREEN}✓ PASS{Colors.END}" if passed else f"{Colors.RED}✗ FAIL{Colors.END}"
    print(f"  [{status}] {name}")
    if details:
        print(f"        {Colors.YELLOW}→ {details}{Colors.END}")

def run_adb(command):
    """运行adb命令并返回输出"""
    try:
        result = subprocess.run(
            f"adb shell {command}",
            shell=True,
            capture_output=True,
            text=True,
            timeout=5
        )
        return result.stdout
    except subprocess.TimeoutExpired:
        return ""
    except Exception as e:
        return ""

def test_process_name():
    """测试1: 检查进程名是否包含'frida'"""
    print_header("测试 1: 进程名检测")
    
    output = run_adb("ps | grep -i frida")
    has_frida = bool(output.strip())
    
    output_fs = run_adb("ps | grep fs-server")
    has_fs_server = bool(output_fs.strip())
    
    print_test(
        "进程列表中无'frida'关键字", 
        not has_frida,
        "找到frida进程" if has_frida else "未找到frida进程"
    )
    
    print_test(
        "fs-server进程正在运行",
        has_fs_server,
        "找到fs-server" if has_fs_server else "未找到fs-server（server可能未运行）"
    )
    
    return not has_frida and has_fs_server

def test_port_detection():
    """测试2: 检查Frida默认端口27042"""
    print_header("测试 2: 端口检测")
    
    output = run_adb("netstat -tuln 2>/dev/null | grep 27042 || ss -tuln 2>/dev/null | grep 27042")
    has_default_port = bool(output.strip())
    
    print_test(
        "未使用默认端口27042",
        not has_default_port,
        "检测到27042端口" if has_default_port else "未检测到默认端口"
    )
    
    return not has_default_port

def test_file_paths():
    """测试3: 检查常见Frida文件路径"""
    print_header("测试 3: 文件路径检测")
    
    suspicious_paths = [
        "/data/local/tmp/frida-server",
        "/data/local/tmp/frida-agent",
        "/data/local/tmp/re.frida.server",
    ]
    
    safe_paths = [
        "/data/local/tmp/fs-server",
    ]
    
    all_passed = True
    
    for path in suspicious_paths:
        output = run_adb(f"ls {path} 2>&1")
        exists = "No such file" not in output and output.strip()
        print_test(
            f"可疑路径不存在: {path}",
            not exists,
            "文件存在！可能被检测" if exists else "文件不存在"
        )
        if exists:
            all_passed = False
    
    for path in safe_paths:
        output = run_adb(f"ls {path} 2>&1")
        exists = "No such file" not in output and output.strip()
        print_test(
            f"混淆路径存在: {path}",
            exists,
            "文件存在" if exists else "文件不存在（server未部署？）"
        )
    
    return all_passed

def test_library_detection():
    """测试4: 检查加载的so库"""
    print_header("测试 4: 库文件检测")
    
    # 获取所有进程的maps
    output = run_adb("cat /proc/*/maps 2>/dev/null | grep -i frida || echo 'not_found'")
    has_frida_lib = "frida" in output.lower() and "not_found" not in output
    
    print_test(
        "内存映射中无'frida'关键字",
        not has_frida_lib,
        "检测到frida相关库" if has_frida_lib else "未检测到frida库"
    )
    
    return not has_frida_lib

def test_tcp_connections():
    """测试5: TCP连接检测"""
    print_header("测试 5: TCP连接检测")
    
    # 检查是否有可疑的TCP连接模式
    output = run_adb("netstat -nap 2>/dev/null | grep -i frida || ss -nap 2>/dev/null | grep -i frida")
    has_frida_connection = bool(output.strip())
    
    print_test(
        "TCP连接中无'frida'标识",
        not has_frida_connection,
        "检测到frida连接" if has_frida_connection else "未检测到frida连接"
    )
    
    return not has_frida_connection

def test_env_variables():
    """测试6: 环境变量检测"""
    print_header("测试 6: 环境变量检测")
    
    output = run_adb("cat /proc/*/environ 2>/dev/null | strings | grep -i frida || echo 'not_found'")
    has_frida_env = "frida" in output.lower() and "not_found" not in output
    
    print_test(
        "环境变量中无'frida'关键字",
        not has_frida_env,
        "检测到frida环境变量" if has_frida_env else "未检测到frida环境变量"
    )
    
    return not has_frida_env

def test_cmdline():
    """测试7: 命令行参数检测"""
    print_header("测试 7: 命令行参数检测")
    
    output = run_adb("cat /proc/*/cmdline 2>/dev/null | strings | grep -i frida || echo 'not_found'")
    has_frida_cmdline = "frida" in output.lower() and "not_found" not in output
    
    print_test(
        "命令行参数中无'frida'关键字",
        not has_frida_cmdline,
        "检测到frida命令行" if has_frida_cmdline else "未检测到frida命令行"
    )
    
    return not has_frida_cmdline

def test_string_patterns():
    """测试8: 常见字符串模式检测"""
    print_header("测试 8: 字符串模式检测")
    
    # 检查server二进制文件中的字符串
    patterns = ["re.frida", "frida-agent", "LIBFRIDA"]
    
    all_passed = True
    for pattern in patterns:
        output = run_adb(f"strings /data/local/tmp/fs-server 2>/dev/null | grep -i '{pattern}' | head -5")
        found = bool(output.strip())
        print_test(
            f"Server中不含模式: {pattern}",
            not found,
            f"找到 {len(output.splitlines())} 处匹配" if found else "未找到"
        )
        if found:
            all_passed = False
    
    # 检查是否使用了re.fs
    output = run_adb("strings /data/local/tmp/fs-server 2>/dev/null | grep 're.fs' | head -3")
    has_renamed = bool(output.strip())
    print_test(
        "Server使用了're.fs'混淆",
        has_renamed,
        "找到re.fs命名空间" if has_renamed else "未找到re.fs（可能未正确编译）"
    )
    
    return all_passed

def test_logcat():
    """测试9: Logcat日志检测"""
    print_header("测试 9: Logcat日志检测")
    
    # 清除旧日志
    run_adb("logcat -c")
    
    # 触发一些可能产生日志的操作（这里只是简单等待，实际上可能需要触发server交互）
    import time
    time.sleep(2)
    
    # 检查日志
    output = run_adb("logcat -d | grep -iE 'frida|gum|bads' | grep -v 'grep' | head -5")
    has_log = bool(output.strip())
    
    print_test(
        "Logcat中无敏感关键字",
        not has_log,
        f"发现日志: {output.strip()[:100]}..." if has_log else "未发现相关日志"
    )
    
    return not has_log

def test_unix_sockets():
    """测试10: Unix域套接字检测"""
    print_header("测试 10: Unix域套接字检测")
    
    output = run_adb("cat /proc/net/unix 2>/dev/null | grep -iE 'frida|gum' || echo 'not_found'")
    has_socket = "frida" in output.lower() or "gum" in output.lower()
    
    print_test(
        "未发现敏感Unix套接字",
        not has_socket,
        "发现可疑套接字" if has_socket else "未发现敏感套接字"
    )
    
    return not has_socket

def test_selinux_context():
    """测试11: SELinux上下文检测"""
    print_header("测试 11: SELinux上下文检测")
    
    # 检查fs-server的上下文
    output = run_adb("ls -Z /data/local/tmp/fs-server 2>/dev/null")
    # 通常frida server会尝试设置特定的上下文，或者被系统分配特定的上下文
    # 这里我们主要检查是否包含 'frida' 相关的上下文标签，如果有的话
    
    has_frida_context = "frida" in output.lower()
    
    print_test(
        "SELinux上下文中无'frida'关键字",
        not has_frida_context,
        f"发现可疑上下文: {output.strip()}" if has_frida_context else "上下文看起来正常"
    )
    
    return not has_frida_context

def test_threads():
    """测试12: 线程特征检测"""
    print_header("测试 12: 线程特征检测")
    
    # 检查所有线程名
    output = run_adb("ps -T | grep -E 'gmain|gdbus|gum-js-loop|pool-frida' || echo 'not_found'")
    has_threads = "gmain" in output or "gum-js-loop" in output
    
    print_test(
        "未发现Frida特征线程",
        not has_threads,
        f"发现可疑线程: {output.strip()[:100]}..." if has_threads else "未发现特征线程"
    )
    
    return not has_threads

def test_tracerpid():
    """测试13: TracerPid检测"""
    print_header("测试 13: TracerPid检测")
    
    # 检查是否有非0的TracerPid (除了调试器自身)
    # 注意: 这里主要检查是否有明显的Frida相关进程在Trace其他进程
    output = run_adb("grep TracerPid /proc/*/status 2>/dev/null | grep -v 'TracerPid:\t0'")
    
    # 如果输出包含 frida 相关的PID (需要结合 ps 输出判断，这里简化处理)
    # 实际上，只要 server 运行，它就会 trace 注入的进程。
    # 这里的检测点是：是否存在被 'fs-server' trace 的进程，且该 trace 行为是否暴露了特征
    
    # 我们检查是否有进程被名为 'frida' 或 'gum' 的进程 trace
    # 由于我们改名为 fs-server，如果 TracerPid 对应的父进程名是 fs-server，那是正常的业务逻辑
    # 但如果是 frida-server，那就是漏网之鱼
    
    has_suspicious_tracer = False
    details = ""
    
    if output.strip():
        lines = output.strip().split('\n')
        for line in lines:
            try:
                # line format: /proc/123/status:TracerPid:	456
                parts = line.split(':')
                pid = parts[0].split('/')[2]
                tracer_pid = parts[2].strip()
                
                if tracer_pid != "0":
                    # 获取 tracer 进程名
                    tracer_name = run_adb(f"cat /proc/{tracer_pid}/comm 2>/dev/null").strip()
                    if "frida" in tracer_name.lower() or "gum" in tracer_name.lower():
                        has_suspicious_tracer = True
                        details = f"发现进程 {pid} 被 {tracer_name}({tracer_pid}) 跟踪"
                        break
            except:
                continue

    print_test(
        "TracerPid未指向Frida相关进程",
        not has_suspicious_tracer,
        details if has_suspicious_tracer else "TracerPid检查通过"
    )
    
    return not has_suspicious_tracer

def test_dbus():
    """测试14: D-Bus痕迹检测"""
    print_header("测试 14: D-Bus痕迹检测")
    
    # 检查 D-Bus 相关的 socket 或连接
    # Frida 使用 D-Bus 通信，可能会在 /proc/net/unix 中留下痕迹
    
    output = run_adb("grep -i 'frida' /proc/net/unix || echo 'not_found'")
    has_frida_dbus = "frida" in output.lower() and "not_found" not in output
    
    print_test(
        "D-Bus中无'frida'痕迹",
        not has_frida_dbus,
        "检测到Frida D-Bus连接" if has_frida_dbus else "未检测到D-Bus痕迹"
    )
    
    return not has_frida_dbus

def test_hardware_spoofing():
    """测试15: 硬件信息伪装检测"""
    print_header("测试 15: 硬件信息伪装检测")
    
    # 检查是否成功伪装成了 Pixel 8 Pro (husky)
    # 注意：这个测试依赖于 hardware_spoofer.so 模块是否生效
    # 我们通过 getprop 获取属性，如果模块生效，应该返回伪装值
    
    # 1. 检查 ro.product.model
    model = run_adb("getprop ro.product.model").strip()
    is_spoofed_model = model == "Pixel 8 Pro"
    
    print_test(
        "设备型号伪装 (Pixel 8 Pro)",
        is_spoofed_model,
        f"当前型号: {model}"
    )
    
    # 2. 检查 ro.debuggable (应为 0)
    debuggable = run_adb("getprop ro.debuggable").strip()
    is_not_debuggable = debuggable == "0"
    
    print_test(
        "调试状态伪装 (ro.debuggable=0)",
        is_not_debuggable,
        f"当前状态: {debuggable}"
    )
    
    # 3. 检查 ro.secure (应为 1)
    secure = run_adb("getprop ro.secure").strip()
    is_secure = secure == "1"
    
    print_test(
        "安全状态伪装 (ro.secure=1)",
        is_secure,
        f"当前状态: {secure}"
    )
    
    return is_spoofed_model and is_not_debuggable and is_secure

def test_modules_loaded():
    """测试16: 验证所有14个保护模块是否加载"""
    print_header("测试 16: 保护模块加载检测")
    
    expected_modules = [
        "env_cleaner.so",
        "thread_name_obfuscator.so",
        "proc_hider.so",
        "antidebug_bypass.so",
        "behavior_randomizer.so",
        "traffic_obfuscator.so",
        "sandbox_bypass.so",
        "memory_protector.so",
        "hook_detector.so",
        "rdtsc_virtualizer.so",
        "chacha20_tls.so",
        "selinux_spoofer.so",
        "art_hook_hider.so",
        "hardware_spoofer.so"
    ]
    
    # 获取 fs-server 的 PID
    pid = run_adb("pidof fs-server").strip()
    if not pid:
        print_test("获取fs-server PID", False, "未找到fs-server进程")
        return False
        
    # 获取 maps
    maps = run_adb(f"cat /proc/{pid}/maps 2>/dev/null")
    
    all_loaded = True
    for module in expected_modules:
        is_loaded = module in maps
        print_test(
            f"模块加载: {module}",
            is_loaded,
            "已加载" if is_loaded else "未加载 (可能被隐藏或加载失败)"
        )
        if not is_loaded:
            all_loaded = False
            
    return all_loaded

def main():
    print(f"\n{Colors.BOLD}{Colors.BLUE}")
    print("╔════════════════════════════════════════════════════════════╗")
    print("║     Frida 反检测测试工具 v1.2                              ║")
    print("║     测试定制编译的Frida Server防检测能力                   ║")
    print("╚════════════════════════════════════════════════════════════╝")
    print(Colors.END)
    
    # 检查adb连接
    print("正在检查ADB连接...")
    result = subprocess.run("adb devices", shell=True, capture_output=True, text=True)
    if "device" not in result.stdout or result.stdout.count("device") < 2:
        print(f"{Colors.RED}错误: 未检测到Android设备，请确保USB调试已开启{Colors.END}")
        sys.exit(1)
    print(f"{Colors.GREEN}✓ ADB连接正常{Colors.END}\n")
    
    # 运行所有测试
    results = {}
    results['process'] = test_process_name()
    results['port'] = test_port_detection()
    results['files'] = test_file_paths()
    results['library'] = test_library_detection()
    results['tcp'] = test_tcp_connections()
    results['env'] = test_env_variables()
    results['cmdline'] = test_cmdline()
    results['strings'] = test_string_patterns()
    results['logcat'] = test_logcat()
    results['unix_sockets'] = test_unix_sockets()
    results['selinux'] = test_selinux_context()
    results['threads'] = test_threads()
    results['tracerpid'] = test_tracerpid()
    results['dbus'] = test_dbus()
    results['hardware'] = test_hardware_spoofing()
    results['modules'] = test_modules_loaded()
    
    # 统计结果
    print_header("测试总结")
    total = len(results)
    passed = sum(1 for v in results.values() if v)
    score = (passed / total) * 100
    
    print(f"  总测试数: {total}")
    print(f"  通过数: {Colors.GREEN}{passed}{Colors.END}")
    print(f"  失败数: {Colors.RED}{total - passed}{Colors.END}")
    print(f"  防检测评分: ", end="")
    
    if score >= 90:
        print(f"{Colors.GREEN}{score:.1f}%{Colors.END} - 优秀！")
    elif score >= 70:
        print(f"{Colors.YELLOW}{score:.1f}%{Colors.END} - 良好")
    else:
        print(f"{Colors.RED}{score:.1f}%{Colors.END} - 需要改进")
    
    print("\n" + "="*60)
    print(f"\n{Colors.BOLD}建议:{Colors.END}")
    
    if not results['process']:
        print(f"  • 确保使用混淆后的server名称运行")
    
    if not results['strings']:
        print(f"  • 考虑进一步混淆二进制文件中的字符串")
    
    if score < 90:
        print(f"  • 检查编译配置，确保所有防检测补丁都已应用")
    else:
        print(f"  {Colors.GREEN}• 防检测效果良好！可以部署使用{Colors.END}")
    
    print()
    return 0 if score >= 70 else 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}测试已中断{Colors.END}")
        sys.exit(1)
