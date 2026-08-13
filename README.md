# MyFolder PC Client

**MyFolder PC Client 是基于 Qt 6 / QML 的 Windows 桌面客户端。** 它负责设备身份维护、设备间文件发送、传输任务管理以及 Windows Shell 集成，并与 MyFolder Server v1.1.1 和 Web 控制台协同工作。

> 当前版本：**v1.1.1**  
> 官网：<https://myfolder.com.cn>  
> API：<https://api.myfolder.com.cn>

## 客户端能力

### 设备与账号

- MyFolder 设备注册、设备身份识别和心跳维护
- 设备列表、设备状态和目标设备选择
- 账号密码登录、会话刷新和退出登录
- Nyauth、Google、GitHub OAuth 提供方状态查询与登录
- OAuth 桌面端回调协议：`myfolder://oauth/callback`
- 会话信息和本地任务状态持久化

### 文件传输

- 选择一个或多个文件发送到其他 MyFolder 设备
- 自动传输通道：`LAN → P2P → Server Relay`
- LAN 局域网直传
- P2P / STUN 连接探测和协商
- 直连不可用时上传到 Server 并由目标设备下载
- 分片上传、断点续传、Range 下载和失败重试
- SHA-256 完整性校验
- 传输进度、任务状态、失败信息和接收通知
- WebSocket 设备通信与实时事件

### Windows 集成

- 文件资源管理器右键“发送到 MyFolder”
- Windows Shell Extension 项目
- 自定义 `myfolder://` URL 协议
- 单实例启动和外部命令转发
- Inno Setup 安装脚本
- MSIX / AppX 相关打包配置

## 分支与服务

当前客户端源码位于 GitHub 的 [`client` 分支](https://github.com/SuoNam/MyFolder/tree/client)。服务端源码和 Web v1.1.1 生产构建产物位于 [`main` 分支](https://github.com/SuoNam/MyFolder/tree/main)。

在线入口：

- 官网：<https://myfolder.com.cn>
- Web 控制台：<https://web.myfolder.com.cn>
- API：<https://api.myfolder.com.cn>

默认 API 地址由 [`serverconfig.h`](serverconfig.h) 提供，可通过环境变量覆盖：

```text
MYFOLDER_BASE_URL=https://api.myfolder.com.cn
```

## 环境要求

- Windows 10/11 x64
- Qt 6.8 或更高版本
- CMake 3.16 或更高版本
- 支持 C++17 的编译器
- Qt 组件：Quick、QuickControls2、Core、Network、WebSockets、Widgets

CMake 工程已经声明项目版本 `1.1.1`，并将版本号编译进桌面应用。

## Windows 构建

在安装 Qt、CMake 和对应编译器后，在客户端源码根目录执行：

```powershell
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

也可以在 Qt Creator 中直接打开根目录的 `CMakeLists.txt`，选择 Qt 6.8+ Desktop Kit 后构建。

构建目标包括：

- `appMyFolder`：MyFolder 桌面应用
- `MyFolderShellExtension`：Windows Shell Extension（仅 Windows）

## 打包

仓库保留以下打包相关内容：

- `packaging/windows/`：AppX、现代右键菜单和 Windows 打包配置
- `installer/`：Inno Setup 安装脚本
- `Icons/`：应用图标资源
- `app.rc`：Windows 应用资源

推荐先构建 Release，再使用 `packaging/windows/` 或 `installer/` 中的脚本制作安装包。已生成的安装包不作为源码提交；正式安装包应通过 GitHub Releases 或官网分发。

## 源码结构

```text
.
├── *.cpp / *.h             # 设备、HTTP、WebSocket、传输和 Windows 集成功能
├── *.qml                   # 主界面、设备、传输、设置和组件界面
├── CMakeLists.txt          # Qt / CMake 构建定义
├── serverconfig.h          # API 地址配置
├── packaging/windows/      # Windows 打包与 Shell 配置
├── installer/              # Inno Setup 安装脚本
└── Icons/                  # 应用图标
```

## 与 Server 的兼容性

本客户端按 MyFolder Server v1.1.1 的接口和传输协议实现。服务端的 API、上传协议和部署文档位于 `main/server/docs/`。

如果使用自建服务端，可以通过 `MYFOLDER_BASE_URL` 指向自己的 API 地址；生产环境应使用 HTTPS，并确保 WebSocket 能够完成安全升级（WSS）。

## 安全提示

- 不要在源码中写入账号密码、JWT、OAuth Client Secret 或其他生产凭据。
- 仅从可信来源安装客户端，并在发布页核对版本和哈希。
- 自建服务时，应为 API、Web 控制台和 OAuth 回调配置正确的 HTTPS 域名。
- 提交 Issue 或日志前，请删除令牌、文件路径、账号和服务器信息。

## 反馈与贡献

欢迎通过 [GitHub Issues](https://github.com/SuoNam/MyFolder/issues) 报告问题或提出建议。请在问题中说明客户端版本、Windows 版本、复现步骤以及经过脱敏的日志。

## 许可证

当前仓库尚未声明开源许可证。在许可证文件发布前，源码的复制、修改与再分发权限不作默认授权。
