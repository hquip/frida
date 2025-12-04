# Frida Stealth 使用指南

## 1. 手机端操作 (服务端)

您不再需要运行官方的 `frida-server`，而是运行我们生成的 `frida-server-stealth`。

### 步骤
```bash
# 1. 推送文件到手机
adb push frida-server-stealth /data/local/tmp/

# 2. 设置权限
adb shell chmod 755 /data/local/tmp/frida-server-stealth

# 3. 运行 (建议在后台运行)
adb shell "/data/local/tmp/frida-server-stealth &"
```

**它会自动做以下事情：**
- 加载所有14个保护模块
- 启动服务 (监听端口 51234)
- 清理痕迹

---

## 2. 电脑端操作 (客户端)

您依然使用电脑上现有的 Frida 工具 (`frida`, `frida-ps`, `frida-trace` 等)，**不需要更换客户端**。

### 关键步骤：端口转发
因为我们将默认端口从 27042 改为了 51234 (为了防检测)，所以需要手动转发端口：

```bash
# 将电脑的 27042 转发到手机的 51234
adb forward tcp:27042 tcp:51234
```

### 然后像平常一样使用：
```bash
# 列出进程
frida-ps -U

# Hook 某个APP
frida -U -f com.example.app -l script.js

# 追踪函数
frida-trace -U -i "open" com.example.app
```

---

## 总结

1. **手机上：** 运行 `frida-server-stealth` (替代原来的 frida-server)
2. **电脑上：** 先运行 `adb forward tcp:27042 tcp:51234`
3. **电脑上：** 继续使用原来的 Frida 工具

**是的，Frida 客户端(电脑端)和 服务端(手机端) 是配合使用的。您只需要替换手机端的服务程序即可。**
