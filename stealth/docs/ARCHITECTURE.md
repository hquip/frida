# Frida Stealth 集成架构说明

## 这是一个"混合集成"方案 (Hybrid Integration)

并不是所有功能都直接编译进 `frida-server` 二进制文件，而是采用了 **静态修改 + 动态注入** 相结合的最佳实践。

### 1. 静态集成 (直接编译进Server) 🏗️
**这些修改直接改在 Frida 源代码中，编译后就在二进制里：**
- ✅ **端口修改:** `socket.vala` (27042 -> 51234)
- ✅ **字符串混淆:** `re.frida` -> `re.fs` (源代码级替换)
- ✅ **进程改名:** `frida-server` -> `fs-server`
- ✅ **版本伪装:** `frida_version.py` 修改

**为什么这样做？**
这些特征是静态的，必须在编译时修改才能彻底去除。

### 2. 动态集成 (LD_PRELOAD注入) 💉
**这14个保护模块是作为外部插件，在启动瞬间注入的：**
- 🛡️ `antidebug_bypass.so` (反调试)
- 🛡️ `proc_hider.so` (隐藏文件)
- 🛡️ `thread_name_obfuscator.so` (隐藏线程)
- ...等14个模块

**为什么不直接编译进去？**
1. **隐蔽性更强:** `env_cleaner.so` 可以完美隐藏注入痕迹，让APP以为是系统行为。
2. **权限更高:** `LD_PRELOAD` 在 libc 初始化之前加载，能 Hook 极其底层的函数（如 `open`, `read`, `ptrace`）。如果编译进 frida 内部，有些底层函数很难 Hook 到自己。
3. **模块化:** 可以根据目标APP的检测手段，灵活开关某些保护（比如有些APP不检测SELinux，就可以关掉该模块以提升性能）。
4. **稳定性:** 修改 Frida 核心 C 代码风险很大，容易导致 Crash。外部注入互不影响。

### 3. 最终效果 🚀
虽然物理上是分开的文件，但在**运行时(Runtime)**，它们是**完全融合**的：

1. 用户运行 `./launch_ultimate_stealth.sh`
2. 脚本设置 `LD_PRELOAD` 加载所有模块
3. 启动 `fs-server`
4. `env_cleaner` **立即清除** `LD_PRELOAD` 环境变量
5. **结果：** 目标APP看到的只是一个普通的进程，看不到注入痕迹，也检测不到 Frida 特征。

**总结：这是目前反检测领域最成熟、最稳健的架构方案。**
