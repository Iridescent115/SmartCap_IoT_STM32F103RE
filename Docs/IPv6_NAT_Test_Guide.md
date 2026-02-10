# IPv6 NAT/防火墙测试指南

## 目的
验证联通4G/5G的IPv6是否真的是公网直连,还是有NAT/防火墙。

---

## 测试1: 验证IPv6地址类型

### 查询RG200U的IPv6地址
```
AT+CGPADDR
```

**示例响应**:
```
+CGPADDR: 1,"100.76.245.80","2408:8441:620:12bc:1891:9b8a:8541:e17b"
```

### 地址分析
- `2408:8441:...` - 这是**真正的公网IPv6** ✅
- 如果是 `fd00::` 或 `fe80::` 开头 - 这是**私有地址** ❌

**您的地址确认**: `2408:8441:...` 是中国联通分配的**公网IPv6**!

---

## 测试2: 从外网ping您的RG200U (关键测试)

### 步骤1: 获取RG200U的IPv6地址
```
AT+CGPADDR
记录下IPv6地址,例如: 2408:8441:620:12bc:1891:9b8a:8541:e17b
```

### 步骤2: 从另一台有IPv6的设备ping
**在家里的电脑上** (确保有IPv6):
```powershell
ping -6 2408:8441:620:12bc:1891:9b8a:8541:e17b
```

**期望结果**:
- ✅ **能ping通** → 说明是真公网,问题可能在RG200U配置
- ❌ **ping不通/超时** → 说明有防火墙阻止ICMP
- ❌ **目标不可达** → 说明有NAT或路由问题

---

## 测试3: 从外网TCP连接到RG200U (终极测试)

### 步骤1: 让RG200U监听一个端口
```
AT+QIOPEN=1,0,"TCP LISTENER","0",0,9999,0
```

**说明**: 让RG200U监听9999端口

### 步骤2: 从电脑尝试连接
**在家里电脑上**:
```powershell
Test-NetConnection -ComputerName "2408:8441:620:12bc:1891:9b8a:8541:e17b" -Port 9999
```

或者用Python:
```python
import socket
s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
s.settimeout(10)
try:
    s.connect(("2408:8441:620:12bc:1891:9b8a:8541:e17b", 9999))
    print("连接成功! ✅ 真公网IPv6")
except:
    print("连接失败! ❌ 有NAT/防火墙")
```

**结果判断**:
- ✅ **连接成功** → IPv6是真公网,可以接受入站连接
- ❌ **连接超时** → 有防火墙/NAT,**不支持入站连接**

---

## 测试4: 验证TCP连接的双向性

### 当前测试结果回顾
已知事实:
- ✅ STM32 → 服务器: **可以发送** (AT+QISEND成功)
- ❌ 服务器 → STM32: **收不到** (AT+QIRD返回0)

### 可能的原因

#### 原因1: 联通CGNAT的单向策略
即使分配了公网IPv6,运营商可能:
- 允许出站流量(客户端主动连接)
- **阻止入站流量**(即使是已建立连接的回复)
- 只允许"回复包"(有状态防火墙)

#### 原因2: RG200U的Buffer模式问题
- Buffer模式(access_mode=0)可能不会触发URC
- 数据在模块缓冲区,但没有通知STM32

#### 原因3: TCP窗口/ACK问题
- STM32发送后打开了NAT通道
- 但服务器的数据包被识别为"新连接"而非"回复"
- 被防火墙拦截

---

## 推荐的解决方案

### 如果测试3失败(不能入站连接)

**立即采用轮询方案**:

1. **HTTP短连接轮询** (最简单)
   - STM32每5秒GET一次 `http://[server]:8080/poll`
   - 服务器有命令就返回,没有就返回204
   - 完全避开NAT问题

2. **MQTT长连接订阅** (标准方案)
   - STM32连接到MQTT Broker并订阅主题
   - 服务器发布到主题,STM32收到
   - 连接由STM32维护(出站),但服务器能"推送"

3. **WebSocket** (高级方案)
   - STM32主动连接WebSocket服务器
   - 连接建立后保持,双向通信
   - 所有数据都是在已建立的连接上

### 如果测试3成功(能入站连接)

**那问题在RG200U配置**:

1. 尝试TCP LISTENER模式让服务器连接STM32
2. 检查是否是Buffer模式的URC触发问题
3. 尝试Direct Push模式(access_mode=1)

---

## 快速验证脚本

### 在RG200U串口执行:
```
# 1. 获取IPv6
AT+CGPADDR

# 2. 监听9999端口
AT+QIOPEN=1,0,"TCP LISTENER","0",0,9999,0

# 等待3-5秒
# 应该收到: +QIOPEN: 0,0

# 3. 检查状态
AT+QISTATE?
```

### 在电脑PowerShell执行:
```powershell
# 替换成实际的RG200U IPv6地址
$ipv6 = "2408:8441:620:12bc:1891:9b8a:8541:e17b"

# 测试连接
Test-NetConnection -ComputerName $ipv6 -Port 9999

# 如果成功,尝试发送数据
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect($ipv6, 9999)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$writer.WriteLine("Hello RG200U!")
$writer.Flush()
```

### 在RG200U串口查看:
```
# 应该收到入站连接通知
+QIURC: "incoming",0

# 读取数据
AT+QIRD=0,1500

# 应该能看到 "Hello RG200U!"
```

---

## 结论

**如果测试3失败** (90%概率):
- 联通虽然给了公网IPv6,但有**严格的入站防火墙**
- 必须改用**轮询模式**(HTTP/MQTT)
- 不能依赖服务器主动推送

**如果测试3成功** (10%概率):
- IPv6真的是端到端直连
- 问题在代码或模块配置
- 可以继续调试TCP方案

---

**建议**: 直接跳过测试,立即实施**HTTP轮询**或**MQTT订阅**方案,这是物联网设备的标准做法!
