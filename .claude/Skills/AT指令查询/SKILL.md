---
name: AT指令查询
description: Quectel RG200U/RM500U series AT command reference and lookup guide. Use this skill whenever working with Quectel 4G/5G modules (RG200U, RM500U series) and AT commands are needed for development, testing, or troubleshooting. This skill provides quick access to AT command syntax, parameters, and usage examples.
---

# Quectel RG200U/RM500U AT 命令参考手册

## 概述

本技能提供 Quectel RG200U/RM500U 系列 4G/5G 模块的 AT 命令快速查找和使用指南。基于 `Quectel_RGx00URM500U系列_AT命令手册_V1.1.pdf`（共 274 页）提取整理。

### 何时使用此技能

在以下场景使用：
- 开发 STM32 与 RG200U 模块通信代码时
- 需要查询特定功能的 AT 命令时
- 调试 AT 命令响应问题时
- 配置 TCP/IP、SMS、网络服务等功能时

### 文档来源

**源文件**: `Docs/RG200U-CN/Quectel_RGx00URM500U系列_AT命令手册_V1.1.pdf`
**总页数**: 274 页
**版本**: V1.1
**日期**: 2024-02-02

## 快速开始

### AT 命令基本格式

```
AT<command>[<parameter>][<CR>]
```

- `AT`：命令前缀
- `<command>`：命令字符串
- `<parameter>`：可选参数
- `<CR>`：回车符（0x0D）

### 响应格式

```
<response><CR><LF>
OK<CR><LF>
```

或错误响应：
```
ERROR<CR><LF>
+CME ERROR: <err><CR><LF>
```

## 主要命令分类

### 1. 通用指令 (General Commands)
- `AT` - 测试 AT 接口
- `ATI` - 查询产品型号 ID 和固件版本信息
- `AT+GMI` - 查询厂商 ID
- `AT+GMM` - 查询产品型号 ID
- `AT+GMR` - 查询产品固件版本
- `AT+GSN` / `AT+CGSN` - 查询国际移动设备识别码（IMEI 号）
- `AT&F` - 恢复 AT 配置为出厂配置
- `AT&W` - 保存当前设置的用户自定义配置到文件
- `ATZ` - 从用户自定义配置文件恢复设备通信参数
- `ATV` - 设置 MT 响应格式
- `ATE` - 设置回显模式
- `AT/` - 重复执行上一条命令

### 2. 通信指令 (Communication Commands)

#### 网络注册和状态
- `AT+CPIN` - 查询 SIM 卡状态
- `AT+CSQ` - 信号质量查询
- `AT+CREG` - 网络注册状态
- `AT+CGATT` - GPRS 附加状态
- `AT+CGACT` - PDP 上下文激活
- `AT+CGDCONT` - 定义 PDP 上下文

#### TCP/IP 相关命令 (重要)
- `AT+QICSGP` - 配置 TCP/IP 参数
- `AT+QIACT` - 激活 PDP 上下文
- `AT+QIDEACT` - 去激活 PDP 上下文
- `AT+QIOPEN` - 打开 socket 连接
- `AT+QICLOSE` - 关闭 socket 连接
- `AT+QISEND` - 通过 socket 发送数据
- `AT+QIRECV` - 接收数据
- `AT+QISTATE` - 查询 socket 状态
- `AT+QICFG` - 配置 TCP/IP 参数
- `AT+QIND` - 注册 URC 指示

### 3. 网络指令 (Network Commands)
- `AT+QNWINFO` - 查询网络信息
- `AT+QNWLOCK` - 锁定运营商
- `AT+QNWLOCKFREQ` - 锁定 LTE/5G 频段
- `AT+QENG` - 查询工程信息

### 4. SIM 卡指令 (SIM Card Commands)
- `AT+CPIN` - 输入 PIN 码
- `AT+CCID` - 查询 SIM 卡 ICCID
- `AT+CPBS` - 选择电话簿存储
- `AT+CPBR` - 读取电话簿条目

### 5. 短信指令 (SMS Commands)
- `AT+CMGF` - 设置短信格式（PDU/Text）
- `AT+CMGS` - 发送短信
- `AT+CMGL` - 列出短信
- `AT+CMGR` - 读取短信
- `AT+CMGD` - 删除短信
- `AT+CNMI` - 新短信指示

### 6. GPIO 指令 (GPIO Commands)
- `AT+QGPIO` - 配置 GPIO
- `AT+QGPIOT` - 设置 GPIO 输出状态
- `AT+QGPIOR` - 读取 GPIO 输入状态

## 查找 AT 命令的步骤

### 步骤 1: 确定功能类别
根据需要实现的功能，确定命令类别：
- **初始化/状态查询**: 通用指令
- **网络连接**: 网络指令、TCP/IP 指令
- **短信收发**: 短信指令
- **硬件控制**: GPIO 指令

### 步骤 2: 查找具体命令
在对应类别中查找相关命令。

### 步骤 3: 查看命令详情
使用以下 Python 代码从 PDF 查看命令详细说明：

```python
import pdfplumber
import json

# 读取索引
with open('Skills/at_commands_index.json', 'r', encoding='utf-8') as f:
    index = json.load(f)

# 搜索命令
search_cmd = 'AT+QIOPEN'  # 替换为你要查找的命令
for key, info in index.items():
    if search_cmd in key.upper():
        page_num = info['page']
        print(f'找到 {key} 在第 {page_num} 页')

        # 从 PDF 提取内容
        pdf_path = 'Docs/RG200U-CN/Quectel_RGx00URM500U系列_AT命令手册_V1.1.pdf'
        with pdfplumber.open(pdf_path) as pdf:
            page = pdf.pages[page_num - 1]
            text = page.extract_text()
            print(text[:2000])
        break
```

## 常用 AT 命令速查

### TCP 客户端连接流程

```bash
# 1. 检查 SIM 卡状态
AT+CPIN?

# 2. 查询信号质量
AT+CSQ

# 3. 查询网络注册状态
AT+CREG?

# 4. 配置 TCP/IP 参数（APN）
AT+QICSGP=1,1,"CMNET","","",1

# 5. 激活 PDP 上下文
AT+QIACT=1

# 6. 打开 TCP 连接
AT+QIOPEN=1,0,"TCP","8.135.10.183",35814,0,1

# 7. 发送数据
AT+QISEND=0,16
> Hello TCP Server

# 8. 接收数据
AT+QIRECV=0,1500

# 9. 关闭连接
AT+QICLOSE=0

# 10. 去激活 PDP
AT+QIDEACT=1
```

### 模块初始化流程

```bash
# 1. 测试 AT 接口
AT

# 2. 查询模块信息
ATI

# 3. 设置回显关闭
ATE0

# 4. 查询信号质量
AT+CSQ

# 5. 查询网络注册状态
AT+CREG?

# 6. 查询 PDP 附着状态
AT+CGATT?
```

### 短信发送流程

```bash
# 1. 设置短信格式为 Text 模式
AT+CMGF=1

# 2. 发送短信
AT+CMGS="13800138000"
> Hello, this is a test message
> (Ctrl+Z 发送)

# 3. 查询已发送短信
AT+CMGL="SENT"
```

## 命令参数说明

### AT+QIOPEN - 打开 socket 连接

**语法**:
```
AT+QIOPEN=<contextID>,<connectID>,"<type>","<IP_address>",<port>[,<access_mode>[,<local_port>]]
```

**参数**:
| 参数 | 说明 |
|------|------|
| `<contextID>` | PDP 上下文 ID，范围 1-16 |
| `<connectID>` | Socket 连接 ID，范围 0-11 |
| `<type>` | 连接类型："TCP" 或 "UDP" |
| `<IP_address>` | 服务器 IP 地址或域名 |
| `<port>` | 服务器端口号 |
| `<access_mode>` | 访问模式：0=缓冲接收，1=直接推送 |
| `<local_port>` | 本地端口号（可选） |

**响应**:
```
OK
+QIOPEN: <connectID>,<err>
```

### AT+QISEND - 发送数据

**语法**:
```
AT+QISEND=<connectID>,<length>
```

**参数**:
| 参数 | 说明 |
|------|------|
| `<connectID>` | Socket 连接 ID |
| `<length>` | 要发送的数据长度 |

**响应**:
```
> (等待数据输入)
SEND OK
```

## 错误代码

### 常见错误代码

| 错误 | 说明 |
|------|------|
| ERROR | 命令执行失败 |
| +CME ERROR: 3 | 操作不允许 |
| +CME ERROR: 4 | 操作不支持 |
| +CME ERROR: 10 | SIM 卡未插入 |
| +CME ERROR: 14 | SIM 卡忙 |
| +CME ERROR: 30 | 没有网络服务 |
| +CME ERROR: 51 | PDP 去激活 |

## 数据文件说明

技能目录包含以下数据文件：

- `at_commands_index.json` - AT 命令索引（898 条）
- `at_commands_toc.txt` - 文档目录
- `at_commands_details.json` - 常用命令详细信息
- `at_chapters.json` - 章节起始位置

## 查询示例

### 查询命令索引

```python
import json

with open('Skills/at_commands_index.json', 'r', encoding='utf-8') as f:
    index = json.load(f)

# 列出所有 TCP 相关命令
for key, info in index.items():
    if 'TCP' in key.upper() or 'IP' in key.upper():
        print(f'{key} - 第 {info["page"]} 页')
```

## 项目相关文件

在 `Firmware/SmartCap_IoT_STM32F103RE/User/user_main/` 目录下：
- `rg200u.c` - RG200U 模块驱动实现
- `rg200u.h` - RG200U 模块驱动头文件

## 参考文档

- `Docs/TCP_IPv6_USAGE.md` - TCP/IPv6 通信使用指南
- `Docs/RG200U_AT_Commands_Manual_Test.md` - AT 命令测试记录
