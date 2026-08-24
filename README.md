# MyFolder PC Client v1.1.1

Qt 6 / QML 桌面客户端，已对接 MyFolder Server v1.1.1。

## 服务端地址

- 默认正式环境：`https://api.myfolder.com.cn`
- 测试环境：启动前设置环境变量 `MYFOLDER_BASE_URL=https://test.myfolder.com.cn`
- `Authorization` 直接传 JWT，不添加 `Bearer` 前缀。

## 已接入功能

- `/user/login`、`/user/signup`：登录与注册。
- `/api/v1/devices`：PC 设备注册、设备令牌、设备列表和心跳。
- `/api/v1/transfers/tasks/*`：4 MiB 分片上传、逐块 SHA-256、整文件 SHA-256、断点续传、文件完成和任务完成。
- `/api/v1/forwards/*`：创建、接受、开始、进度、P2P 信令、完成、失败和取消转发。
- `/device`：WSS 实时设备列表、心跳和转发事件。
- `/api/v1/forwards/{id}/files/content`：接收端 Range 分段下载、续传和 SHA-256 校验。
- `/file/downloadfile`：服务器文件下载。

“发送给设备”按 `LAN → P2P → RELAY` 选择真实通道。LAN 使用客户端 TCP V2 协议；P2P 使用 libdatachannel 的 ICE/STUN 与 DTLS/SCTP DataChannel；直连失败时自动取消原任务并转入 RELAY。

## 构建

需要 Qt 6.8 或更高版本、CMake、C++17、vcpkg 与 `libdatachannel`，以及以下 Qt 模块：

- Quick
- QuickControls2
- Core
- Network
- WebSockets
- Widgets

不再依赖 CURL；旧的 `ftp.cpp`、`ftp.h` 仅作为历史文件保留，不参与构建。

```bash
vcpkg install libdatachannel:x64-mingw-static
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=G:/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static
cmake --build build --config Release
```

`vcpkg.json` 已锁定原生 P2P 依赖。普通 CTest 不运行依赖本机 UDP/防火墙条件的 ICE 网络测试；允许本机 UDP 回环后可加 `-DMYFOLDER_ENABLE_P2P_NETWORK_TESTS=ON` 显式启用。

QML 模块 URI 为 `MyTest`，入口为 `engine.loadFromModule("MyTest", "Main")`。
