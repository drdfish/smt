
---

# 智能家居环境监控系统

这是一个基于 C 语言实现的客户端-服务器（C/S）架构的智能家居环境监控系统。该项目旨在演示如何结合使用 TCP 和 UDP 套接字来构建一个既能保证控制指令可靠传输，又能实现传感器数据实时高效上报的物联网（IoT）应用原型。

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## ✨ 主要特性

- **混合协议通信**：
    - **TCP** 用于控制通道，确保用户命令（如查询设备、执行操作）和服务器响应（如报警信息）的可靠传输。
    - **UDP** 用于数据通道，允许模拟的传感器设备以低延迟、高效率的方式上报实时环境数据。
- **高并发处理**：服务器采用 `select` I/O 多路复用模型，能够在单线程内高效地处理多个客户端的 TCP 连接和 UDP 数据报，资源占用低。
- **实时报警机制**：服务器能够根据预设规则（例如温度超过阈值）对传感器数据进行分析，并主动通过 TCP 连接向控制客户端推送实时报警信息。
- **跨平台构建**：使用 CMake 作为构建系统，支持在 Linux, macOS 和 Windows 等主流操作系统上轻松编译和运行。
- **异步客户端交互**：控制客户端采用多线程设计，将用户输入（UI）与网络消息接收分离，确保在用户输入命令时不会错过服务器推送的紧急报警。

## 🏗️ 系统架构

系统由三个核心组件构成：

1.  **服务器 (Server)**：作为系统的中心枢纽，同时监听一个 TCP 端口（用于控制）和一个 UDP 端口（用于数据）。它负责管理所有客户端连接、存储传感器数据、解析命令并触发报警。
2.  **传感器客户端 (Sensor Client - UDP)**：模拟部署在家庭环境中的传感器设备。它通过 UDP 协议，以无连接的方式定期向服务器发送模拟的环境数据（如温度）。
3.  **控制客户端 (Control Client - TCP)**：为用户提供一个命令行交互界面。用户通过此客户端与服务器建立稳定的 TCP 连接，发送管理命令并接收数据响应和异步报警。

```
+-----------------------------+                           +-------------------------+
|      控制客户端 (TCP)       |  <-- TCP可靠连接 -->      |                         |
|  (User Control Interface)   |      (命令/响应/报警)      |                         |
| - 发送 ls, exec 等命令      |--------------------------->|                         |
| - 接收设备列表、数据、报警  |                           |     智能家居服务器      |
+-----------------------------+                           | (TCP Listener + UDP Port) |
                                                          |                         |
                                                          | - I/O多路复用 (select) |
+-----------------------------+                           | - 管理客户端连接        |
|      传感器客户端 (UDP)     |  <-- UDP实时数据 -->      | - 存储/处理传感器数据   |
|      (Sensor Device)      |      (温度数据等)          | - 触发报警逻辑          |
| - 模拟生成传感器数据        |--------------------------->|                         |
| - 定期发送数据              |                           |                         |
+-----------------------------+                           +-------------------------+
```

## 📁 项目结构

```
.
├── CMakeLists.txt              # 根 CMakeLists 文件
├── ClientProject/
│   ├── CMakeLists.txt
│   └── client.c                # 客户端 (TCP控制 + UDP传感器) 源码
├── ServerProject/
│   ├── CMakeLists.txt
│   └── server.c                # 服务器源码
└── src/
    ├── CMakeLists.txt
    ├── network_utils.c         # 跨平台网络工具函数实现
    └── network_utils.h         # 网络工具头文件和协议定义
```

## 🚀 开始使用

### 📋 环境要求

- **CMake**: 版本 3.15 或更高
- **C 编译器**:
    - **Linux/macOS**: GCC / Clang
    - **Windows**: Visual Studio (MSVC) 或 MinGW-w64

### 🛠️ 编译指南

#### 在 Linux / macOS 上

1.  克隆或下载本仓库。
2.  打开终端，进入项目根目录。
3.  创建构建目录并配置项目：
    ```bash
    cmake -B build
    ```
4.  编译项目：
    ```bash
    cmake --build build
    ```
    编译成功后，可执行文件 `server` 和 `client` 将位于项目根目录下的 `bin` 文件夹中。

#### 在 Windows 上

1.  克隆或下载本仓库。
2.  打开 **"Developer Command Prompt for VS"** (推荐) 或一个配置好 MinGW 的终端。
3.  进入项目根目录。
4.  创建构建目录并配置项目：
    ```cmd
    REM 使用 Visual Studio
    cmake -B build

    REM 或者使用 MinGW
    cmake -B build -G "MinGW Makefiles"
    ```
5.  编译项目：
    ```cmd
    cmake --build build
    ```
    编译成功后，可执行文件 `server.exe` 和 `client.exe` 将位于项目根目录下的 `bin` 文件夹中。

### ⚡ 运行系统

请打开**三个**独立的终端窗口来模拟完整的系统交互。

1.  **终端 1: 启动服务器**
    服务器将开始监听 TCP 端口 `8080` 和 UDP 端口 `8081`。
    ```bash
    # Linux / macOS
    ./bin/server

    # Windows
    .\bin\server.exe
    ```

2.  **终端 2: 启动传感器客户端 (UDP)**
    此客户端将模拟 `DEV0` 设备，通过 UDP 向服务器发送 5 次温度数据。
    ```bash
    # Linux / macOS
    ./bin/client

    # Windows
    .\bin\client.exe
    ```
    此时，您应该能在服务器终端看到接收到传感器数据的日志。

3.  **终端 3: 启动控制客户端 (TCP)**
    启动 TCP 控制台，用于与服务器进行交互。
    ```bash
    # Linux / macOS
    ./bin/client control

    # Windows
    .\bin\client.exe control
    ```

### ⌨️ 控制台命令

连接成功后，您将在控制客户端看到 `>>` 提示符。输入 `help` 查看所有可用命令：

```
>> help
可用命令:
  ls                  : 列出所有在线设备
  ls -s <device_id>   : 查询指定设备的传感器数据
  conf <device_id>    : 配置指定设备参数
  exec <device_id>    : 在指定设备上执行远程命令
  exit                : 退出客户端
```
