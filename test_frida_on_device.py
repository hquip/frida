#!/usr/bin/env python3
"""
设备端Frida检测脚本（可以在Android设备上直接运行）
用于从应用内部检测Frida是否存在
"""

import os
import sys
import subprocess

class FridaDetector:
    """Frida检测器 - 常见的应用内检测方法"""
    
    def __init__(self):
        self.detection_results = []
    
    def check_process_name(self):
        """检测1: 检查进程列表"""
        print("[*] 检查进程名...")
        try:
            # 检查常见的frida进程名
            suspicious_names = [
                "frida-server", 
                "frida-agent",
                "frida",
                "re.frida.server"
            ]
            
            # 读取所有进程
            processes = []
            for pid_dir in os.listdir('/proc'):
                if pid_dir.isdigit():
                    try:
                        cmdline_path = f'/proc/{pid_dir}/cmdline'
                        with open(cmdline_path, 'r') as f:
                            cmdline = f.read().replace('\x00', ' ')
                            processes.append(cmdline.lower())
                    except:
                        pass
            
            # 检查是否有可疑进程
            found = []
            for proc in processes:
                for name in suspicious_names:
                    if name in proc:
                        found.append(name)
            
            if found:
                print(f"    [!] 检测到Frida进程: {', '.join(set(found))}")
                return True
            else:
                print(f"    [✓] 未检测到常见Frida进程名")
                return False
        except Exception as e:
            print(f"    [?] 检测失败: {e}")
            return None
    
    def check_ports(self):
        """检测2: 检查常见端口"""
        print("[*] 检查Frida默认端口...")
        try:
            # Frida默认端口
            frida_ports = [27042, 27043]
            
            # 读取tcp连接
            found_ports = []
            try:
                with open('/proc/net/tcp', 'r') as f:
                    for line in f:
                        parts = line.split()
                        if len(parts) > 1:
                            local_address = parts[1]
                            port = int(local_address.split(':')[1], 16)
                            if port in frida_ports:
                                found_ports.append(port)
            except:
                pass
            
            if found_ports:
                print(f"    [!] 检测到Frida端口: {found_ports}")
                return True
            else:
                print(f"    [✓] 未检测到默认Frida端口")
                return False
        except Exception as e:
            print(f"    [?] 检测失败: {e}")
            return None
    
    def check_library_injection(self):
        """检测3: 检查加载的.so库"""
        print("[*] 检查注入的库...")
        try:
            pid = os.getpid()
            maps_path = f'/proc/{pid}/maps'
            
            suspicious_libs = [
                'frida',
                'frida-agent',
                'frida-gadget',
                'libfrida'
            ]
            
            found_libs = []
            with open(maps_path, 'r') as f:
                for line in f:
                    for lib in suspicious_libs:
                        if lib in line.lower():
                            # 提取库名
                            parts = line.strip().split()
                            if len(parts) >= 6:
                                found_libs.append(parts[-1])
            
            if found_libs:
                print(f"    [!] 检测到Frida库: {', '.join(set(found_libs))}")
                return True
            else:
                print(f"    [✓] 未检测到Frida相关库")
                return False
        except Exception as e:
            print(f"    [?] 检测失败: {e}")
            return None
    
    def check_frida_files(self):
        """检测4: 检查常见Frida文件"""
        print("[*] 检查Frida相关文件...")
        
        suspicious_paths = [
            '/data/local/tmp/frida-server',
            '/data/local/tmp/re.frida.server',
            '/data/local/tmp/frida-agent',
            '/sdcard/frida-server',
        ]
        
        found_files = []
        for path in suspicious_paths:
            if os.path.exists(path):
                found_files.append(path)
        
        if found_files:
            print(f"    [!] 检测到Frida文件: {', '.join(found_files)}")
            return True
        else:
            print(f"    [✓] 未检测到常见Frida文件")
            return False
    
    def check_d_bus(self):
        """检测5: 检查D-Bus接口（需要root）"""
        print("[*] 检查D-Bus接口...")
        try:
            # 这个检测需要root权限，所以可能失败
            result = subprocess.run(
                ['dbus-send', '--print-reply', '--dest=org.freedesktop.DBus', 
                 '/org/freedesktop/DBus', 'org.freedesktop.DBus.ListNames'],
                capture_output=True,
                text=True,
                timeout=2
            )
            
            if 're.frida' in result.stdout.lower():
                print(f"    [!] 检测到re.frida D-Bus接口")
                return True
            else:
                print(f"    [✓] 未检测到re.frida接口")
                return False
        except:
            print(f"    [?] 无法检测D-Bus（可能需要root权限）")
            return None
    
    def check_tracerpid(self):
        """检测6: 检查TracerPid（反调试）"""
        print("[*] 检查TracerPid...")
        try:
            pid = os.getpid()
            status_path = f'/proc/{pid}/status'
            
            with open(status_path, 'r') as f:
                for line in f:
                    if line.startswith('TracerPid:'):
                        tracer_pid = int(line.split(':')[1].strip())
                        if tracer_pid != 0:
                            print(f"    [!] 检测到调试器 (TracerPid: {tracer_pid})")
                            return True
            
            print(f"    [✓] 未检测到调试器")
            return False
        except Exception as e:
            print(f"    [?] 检测失败: {e}")
            return None
    
    def check_threads(self):
        """检测7: 检查可疑线程名"""
        print("[*] 检查可疑线程...")
        suspicious_threads = ['gmain', 'gdbus', 'gum-js-loop', 'pool-frida', 'linjector']
        found_threads = []
        
        try:
            # 遍历所有进程的任务/线程
            for pid in os.listdir('/proc'):
                if not pid.isdigit(): continue
                
                try:
                    task_dir = f'/proc/{pid}/task'
                    if not os.path.exists(task_dir): continue
                    
                    for tid in os.listdir(task_dir):
                        comm_path = f'{task_dir}/{tid}/comm'
                        try:
                            with open(comm_path, 'r') as f:
                                thread_name = f.read().strip()
                                if thread_name in suspicious_threads:
                                    found_threads.append(f"{thread_name} (pid: {pid})")
                        except: pass
                except: pass
                
            if found_threads:
                print(f"    [!] 检测到Frida相关线程: {', '.join(found_threads)}")
                return True
            else:
                print(f"    [✓] 未检测到可疑线程")
                return False
        except Exception as e:
            print(f"    [?] 线程检测失败: {e}")
            return None

    def check_named_pipes(self):
        """检测8: 检查命名管道/Unix域套接字"""
        print("[*] 检查命名管道...")
        suspicious_patterns = ['frida', 'gum', 'linjector']
        found_pipes = []
        
        try:
            with open('/proc/net/unix', 'r') as f:
                for line in f:
                    for pattern in suspicious_patterns:
                        if pattern in line.lower():
                            found_pipes.append(line.strip().split()[-1])
            
            if found_pipes:
                print(f"    [!] 检测到可疑管道: {', '.join(set(found_pipes))[:100]}...")
                return True
            else:
                print(f"    [✓] 未检测到可疑管道")
                return False
        except Exception as e:
            print(f"    [?] 管道检测失败: {e}")
            return None

    def check_directories(self):
        """检测9: 检查特定目录痕迹"""
        print("[*] 检查目录痕迹...")
        suspicious_dirs = [
            '/data/local/tmp/re.frida.server', 
            '/data/local/tmp/frida-agent',
            '/data/local/tmp/frida-gadget'
        ]
        
        # 检查 /data/local/tmp 下的任何以 frida 开头的文件
        try:
            for fname in os.listdir('/data/local/tmp'):
                if fname.startswith('frida') or fname.startswith('re.frida'):
                    suspicious_dirs.append(f"/data/local/tmp/{fname}")
        except: pass

        found = []
        for path in suspicious_dirs:
            if os.path.exists(path):
                found.append(path)
        
        if found:
            print(f"    [!] 检测到残留文件: {', '.join(found)}")
            return True
        else:
            print(f"    [✓] 未检测到目录痕迹")
            return False

    def run_all_checks(self):
        """运行所有检测"""
        print("="*60)
        print("Frida检测器 - 深度全量检测")
        print("="*60)
        print()
        
        checks = [
            ("进程名检测", self.check_process_name),
            ("端口检测", self.check_ports),
            ("库注入检测", self.check_library_injection),
            ("文件系统检测", self.check_frida_files),
            ("D-Bus检测", self.check_d_bus),
            ("TracerPid检测", self.check_tracerpid),
            ("线程特征检测", self.check_threads),
            ("命名管道检测", self.check_named_pipes),
            ("目录痕迹检测", self.check_directories),
        ]
        
        results = []
        for name, check_func in checks:
            try:
                result = check_func()
                results.append((name, result))
            except Exception as e:
                print(f"[!] {name}执行失败: {e}")
                results.append((name, None))
            print()
        
        # 统计结果
        print("="*60)
        print("检测结果总结")
        print("="*60)
        
        detected = sum(1 for _, r in results if r is True)
        not_detected = sum(1 for _, r in results if r is False)
        unknown = sum(1 for _, r in results if r is None)
        
        print(f"\n检测到Frida特征: {detected} 项")
        print(f"未检测到: {not_detected} 项")
        print(f"未知/失败: {unknown} 项\n")
        
        if detected > 0:
            print("[!] 警告: 环境中存在Frida特征！")
            print("\n发现的特征:")
            for name, result in results:
                if result is True:
                    print(f"  - {name}")
            return 1
        else:
            print("[✓] 恭喜: 未检测到任何Frida特征！")
            return 0

def main():
    detector = FridaDetector()
    return detector.run_all_checks()

if __name__ == "__main__":
    sys.exit(main())
