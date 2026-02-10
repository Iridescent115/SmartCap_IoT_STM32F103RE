# TCP/IPv6通信系统使用说明

## 项目概述

本项目实现了STM32 + RG200U 4G模块通过TCP/IPv6与服务器进行双向通信。

**系统架构：**
```
[Python服务器] <--TCP/IPv6--> [RG200U 4G模块] <--UART--> [STM32] <--RS485--> [上位机]
```

## 一、准备工作

### 1. 网络环境配置

#### 光猫和路由器IPv6设置（已完成）
- ✅ 光猫开启IPv6
- ✅ 路由器获取IPv6公网地址
- ⚠️ 需要记录你的IPv6公网地址

#### 查看IPv6地址
**Windows命令：**
```cmd
ipconfig
```
查找类似这样的地址：`2400:3200:1600:800::1`

**Linux/Mac命令：**
```bash
ip addr
# 或
ifconfig
```

### 2. 修改服务器地址

编辑 `rg200u.h` 文件，修改服务器IPv6地址和端口：

```c
#define TCP_SERVER_IP    "2400:3200:1600:800::1"  /* 改为你的IPv6地址 */
#define TCP_SERVER_PORT  "8080"                    /* 服务器端口 */
```

重新编译并下载到STM32。

## 二、启动服务器

### 1. 安装Python依赖

```bash
pip install PyQt5
```

### 2. 运行服务器

```bash
cd Server
python tcp_server.py
```

### 3. 启动监听

1. 确认监听地址为 `::`（IPv6任意地址）
2. 确认端口为 `8080`（或你自定义的端口）
3. 点击 **"启动服务器"** 按钮
4. 等待客户端连接...

## 三、STM32端操作

### 1. 上电启动

STM32上电后会自动执行以下流程：

1. 初始化RG200U模块
2. 测试AT指令
3. 查询网络注册状态
4. 查询信号强度
5. 获取运营商信息
6. 获取IPv4/IPv6地址
7. **自动连接TCP服务器**
8. 启动TCP消息监听

### 2. 查看连接状态

通过RS485串口助手查看启动日志：

```
==================================
  RG200U 4G Gateway Starting...
==================================
Waiting for RG200U to boot up (8s)...

=== RG200U 4G Module Self-Test ===

[1/5] Testing AT command...OK
[2/5] Checking network registration...Registered (Roaming)
[3/5] Checking signal strength...RSSI: -73 dBm (Good)
[4/5] Getting operator info...中国移动
[5/5] Getting IP addresses...
  IPv4: 10.123.45.67
  IPv6: 2408:xxxx:xxxx:xxxx::1

[TCP] Connecting to server...
[TCP] Connected to 2400:3200:1600:800::1:8080
[TCP] TCP transparent mode enabled.
```

如果看到 `[TCP] Connected to...` 说明连接成功！

## 四、通信测试

### 1. 服务器→STM32→上位机

在Python服务器界面：

1. 在"发送数据"输入框输入：`Hello STM32`
2. 点击"发送"按钮

STM32会通过RS485转发到上位机，显示：
```
[TCP RX] Hello STM32
```

### 2. 快捷控制测试

点击服务器界面的快捷按钮：

- **测试消息**：发送 `Hello from Server!`
- **继电器1开**：发送 `RELAY1_ON`
- **继电器1关**：发送 `RELAY1_OFF`

上位机会收到相应的控制指令。

### 3. STM32→服务器（未来扩展）

当前版本：STM32主要接收服务器数据并通过RS485转发。

未来可添加：上位机通过RS485发送数据 → STM32 → RG200U → 服务器。

## 五、工作原理

### TCP连接建立

```c
// RG200U_Init() 中自动执行
AT+QIOPEN=1,0,"TCP","2400:3200:1600:800::1",8080,0,0

// 响应：
OK
+QIOPEN: 0,0  // Socket 0, 错误码0表示成功
```

### 接收服务器数据

1. **服务器发送数据** → RG200U接收
2. **RG200U发送URC通知**：`+QIURC: "recv",0`
3. **STM32检测到通知** → 调用 `RG200U_ProcessTCPMessage()`
4. **读取数据**：`AT+QIRD=0,1500`
5. **转发到RS485** → 上位机接收

### 代码执行流程

```
UserTask_RG200U_RxHandler (FreeRTOS任务, 1ms周期)
  ↓
RG200U_ProcessTCPMessage()  // 检测+QIURC通知
  ↓
检测到 "+QIURC: \"recv\",0"
  ↓
RG200U_ReadTCPData()  // AT+QIRD=0,1500
  ↓
RS485_SendBuffer()  // 转发到上位机
```

## 六、故障排查

### 问题1：服务器无法启动

**症状：** 提示端口已被占用

**解决：**
```bash
# Windows查找占用端口的程序
netstat -ano | findstr 8080
taskkill /F /PID <进程ID>
```

### 问题2：STM32无法连接服务器

**可能原因：**

1. **IPv6地址错误**
   - 检查 `rg200u.h` 中的 `TCP_SERVER_IP`
   - 确保是服务器的IPv6公网地址

2. **RG200U没有IPv6地址**
   - 查看启动日志，确认获取了IPv6地址
   - 检查SIM卡是否支持IPv6（中国移动/联通/电信大部分支持）

3. **防火墙阻止**
   - Windows防火墙允许8080端口入站
   - 路由器防火墙允许IPv6访问

4. **网络未注册**
   - 查看启动日志 `[2/5] Checking network registration`
   - 应显示 `Registered` 或 `Registered (Roaming)`

### 问题3：连接成功但收不到数据

**检查步骤：**

1. 服务器端显示客户端已连接
2. STM32启动日志显示 `[TCP] Connected`
3. 查看 `UserTask_RG200U_RxHandler` 是否在运行
4. 检查RS485连接和波特率（115200）

## 七、代码结构

### STM32端

```
rg200u.h / rg200u.c
├── RG200U_Init()              // 初始化并自动连接服务器
├── RG200U_ConnectTCPServer()  // 连接TCP服务器
├── RG200U_GetTCPState()       // 获取连接状态
├── RG200U_ReadTCPData()       // 读取TCP数据
└── RG200U_ProcessTCPMessage() // 处理+QIURC通知

user_tasks.c
└── UserTask_RG200U_RxHandler()  // 定期调用ProcessTCPMessage
```

### Python服务器端

```
tcp_server.py
├── TCPServer (主窗口类)
│   ├── start_server()      // 启动TCP监听
│   ├── server_loop()       // 接受连接和接收数据
│   ├── send_data()         // 发送数据到客户端
│   └── quick_send()        // 快捷发送
└── ServerSignals (信号类)
    ├── log_message         // 日志信号
    ├── client_connected    // 客户端连接信号
    ├── client_disconnected // 客户端断开信号
    └── data_received       // 数据接收信号
```

## 八、下一步扩展

### 功能扩展建议

1. **上位机→服务器通信**
   - 在 `UserTask_RS485_RxHandler` 中检测特殊指令
   - 调用 `AT+QISEND` 发送数据到服务器

2. **JSON协议**
   - 定义标准JSON格式消息
   - 服务器发送：`{"cmd":"relay","id":1,"state":"on"}`
   - STM32解析并执行控制

3. **心跳保活**
   - 定时发送心跳包保持连接
   - 服务器检测超时断开

4. **断线重连**
   - 检测TCP连接断开
   - 自动重新连接服务器

5. **数据加密**
   - 使用TLS/SSL加密通信（RG200U支持）
   - 防止数据被窃听

## 九、技术参数

| 参数 | 值 |
|------|-----|
| 网络协议 | TCP/IPv6 |
| 服务器端口 | 8080（可配置） |
| 数据编码 | UTF-8 |
| UART波特率 | 115200 |
| 最大数据包 | 1500字节 |
| Socket超时 | 30秒 |
| FreeRTOS任务周期 | 1ms |

## 十、联系和支持

如遇问题，请检查：
1. 启动日志是否完整
2. IPv6地址是否正确
3. 防火墙是否开放
4. SIM卡是否正常注册

开发日期：2026年2月6日
