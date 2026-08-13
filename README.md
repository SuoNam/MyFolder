# MyFolder

**MyFolder 是面向个人多设备环境的文件管理与设备间文件流转平台。** 通过桌面客户端、Web 控制台和中心服务，用户可以管理自己的设备与文件，并在 LAN、P2P 和服务器中转通道之间自动选择合适的传输路径。

> 当前公开版本：**v1.1.1**  
> 官方网站：<https://myfolder.com.cn>

## 在线服务

| 服务 | 地址 | 说明 |
|---|---|---|
| 官方网站 | <https://myfolder.com.cn> | 产品介绍与下载入口 |
| Web 控制台 | <https://web.myfolder.com.cn> | 文件、设备、群组、传输与账号管理 |
| API 服务 | <https://api.myfolder.com.cn> | MyFolder Server 公网 API |

## v1.1.1 功能概览

### 多设备与文件传输

- 同一账号下的设备注册、心跳、在线状态、重命名与移除
- 设备详情、系统信息、客户端版本和最近在线时间展示
- 从一台设备向另一台设备发送单个或多个文件
- 自动通道路由：`LAN → P2P → Relay`
- LAN 局域网高速直传
- P2P / STUN 探测与点对点传输协商
- 直连不可用时自动使用服务器中转
- 分片上传、断点续传、Range 下载、失败重试与任务恢复
- 文件清单与 SHA-256 完整性校验
- WebSocket 实时通知、进度同步和传输历史
- Windows 右键菜单“发送到 MyFolder”集成

### 文件、群组与共享存储

- 私有空间与群组空间
- 文件和目录的创建、上传、下载、重命名、移动与删除
- 文件名冲突处理、批量操作与回收站操作
- 群组创建、成员邀请、权限调整和成员移除
- 群组角色：`READ`、`WRITE`、`OWNER`
- 文件夹 ACL：`NONE`、`READ`、`WRITE`、`MANAGE`
- 存储用量、配额和配额预留

### 账号、安全与 OAuth

v1.1.1 对账号体系进行了完整升级，包含：

- 账号或邮箱 + 密码登录
- 邮箱验证码注册、找回密码与重置密码
- Access Token + Refresh Token 可撤销会话
- Web 与桌面端独立的会话生命周期
- **Nyauth、Google、GitHub OAuth 登录**
- OAuth `state`、PKCE 和 OIDC `nonce` 校验
- 短时、一次性 OAuth 登录交换凭据
- 第三方账号绑定与解除绑定
- OAuth 绑定前的邮箱验证码二次认证
- 敏感操作短时凭据
- 防止解除账号的最后一种可用登录方式
- Web OAuth 回调页与桌面端 `myfolder://oauth/callback` 回调

详细更新内容见 [v1.1.1 发布说明](docs/RELEASE_NOTES_v1.1.1.md)。

## 分支说明

| 分支 | 内容 |
|---|---|
| [`main`](https://github.com/SuoNam/MyFolder/tree/main) | MyFolder Server 源码和 Web v1.1.1 部署产物 |
| [`client`](https://github.com/SuoNam/MyFolder/tree/client) | Qt 6 / QML 桌面客户端源码与 Windows 打包配置 |
| [`dev`](https://github.com/SuoNam/MyFolder/tree/dev) | 历史开发内容；不保证与当前发布版同步 |

## main 分支目录

```text
.
├── server/                 # Spring Boot 服务端源码
│   ├── docs/               # API、上传协议与部署文档
│   ├── src/                # Java 源码、测试与 Flyway 迁移
│   ├── .env.example        # 环境变量示例；不得写入真实密钥
│   ├── mvnw / mvnw.cmd
│   └── pom.xml
├── web/                    # Vue Web 控制台的生产构建产物
│   ├── assets/
│   └── index.html
└── docs/
    └── RELEASE_NOTES_v1.1.1.md
```

Web 目录是与 v1.1.1 官方部署对应的静态构建结果，不包含 `node_modules` 或前端开发缓存。

## 技术栈

### Server

- Java 17
- Spring Boot 3.4
- Spring Security
- MyBatis
- MySQL
- Redis
- Flyway
- WebSocket
- Springdoc OpenAPI
- Maven Wrapper

### Web

- Vue 3
- TypeScript
- Vite
- 静态 SPA 部署

### Desktop Client

客户端源码位于 [`client` 分支](https://github.com/SuoNam/MyFolder/tree/client)：

- C++17
- Qt 6.8+
- QML / Qt Quick Controls
- Qt Network / WebSockets / Widgets
- CMake
- Windows Shell Extension 与 Inno Setup

## 本地运行 Server

### 1. 环境要求

- JDK 17
- MySQL
- Redis

### 2. 准备配置

进入 `server`，复制示例配置：

```bash
cd server
cp .env.example .env
```

`.env` 仅作为配置清单示例。Spring Boot 的实际部署可通过操作系统环境变量、容器 Secret 或部署平台的密钥管理功能注入配置。

至少需要准备数据库、Redis、JWT 和存储目录相关变量。启用邮件与 OAuth 时，还需要按需配置：

```text
MYFOLDER_RESEND_API_KEY
MYFOLDER_RESEND_FROM
MYFOLDER_PUBLIC_API_URL
MYFOLDER_WEB_URL
MYFOLDER_OAUTH_NYAUTH_CLIENT_ID
MYFOLDER_OAUTH_NYAUTH_CLIENT_SECRET
MYFOLDER_OAUTH_GOOGLE_CLIENT_ID
MYFOLDER_OAUTH_GOOGLE_CLIENT_SECRET
MYFOLDER_OAUTH_GITHUB_CLIENT_ID
MYFOLDER_OAUTH_GITHUB_CLIENT_SECRET
```

不要把真实数据库密码、JWT Secret、OAuth Client Secret、邮件 API Key 或 TLS 私钥提交到仓库。

### 3. 启动开发服务

Linux / macOS：

```bash
cd server
./mvnw spring-boot:run
```

Windows：

```powershell
cd server
.\mvnw.cmd spring-boot:run
```

默认开发端口由 `application-development.properties` 设置为 `8082`，可通过 `MYFOLDER_SERVER_PORT` 覆盖。

### 4. 测试与构建

Linux / macOS：

```bash
cd server
./mvnw clean test
./mvnw clean package
```

Windows：

```powershell
cd server
.\mvnw.cmd clean test
.\mvnw.cmd clean package
```

构建产物名称为：

```text
MyFolder_Sever-1.1.1.jar
```

项目内部历史命名使用了 `Sever`，因此 Maven artifact 和 Java 包名继续保留该拼写；面向用户的文档统一称为 Server。

## 数据库迁移

Server 使用 Flyway 管理数据库结构。v1.1.1 包含账号认证、用户显示名称、群组、ACL、配额和配额预留等迁移。生产环境升级前应先备份数据库，并在与生产一致的测试环境验证迁移。

## Web 部署

`web/` 可以直接部署到 Nginx、对象存储静态网站或其他静态托管平台。由于它是单页应用，Web Server 需要把未知前端路由回退到 `index.html`。

Nginx 示例：

```nginx
server {
    listen 443 ssl http2;
    server_name web.example.com;

    root /var/www/myfolder-web;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    location /assets/ {
        try_files $uri =404;
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

如果自行重新构建 Web，请确保生产 API 地址、OAuth 回调地址和 CORS 配置指向同一套受信任环境。

## API 文档

Server 启动后可访问：

```text
/swagger-ui.html
/v3/api-docs
```

详细协议说明也可参阅 [`server/docs`](server/docs)。

## 安全建议

- 生产环境必须使用 HTTPS / WSS。
- JWT Secret 应使用足够长度的随机值并通过 Secret 管理系统注入。
- OAuth 回调地址必须与提供方控制台中的配置严格一致。
- 反向代理终止 TLS 时，应限制后端端口只允许本机或可信私网访问。
- 数据库、Redis、存储目录和日志目录应使用最小权限账户。
- 升级前备份数据库和文件元数据，并验证 Flyway 迁移。
- 仓库只保留 `.env.example`，绝不提交真实 `.env`、证书或密钥。

## 参与项目

欢迎通过 [GitHub Issues](https://github.com/SuoNam/MyFolder/issues) 报告问题、提出功能建议或提交改进方案。提交问题时请附上使用版本、操作系统、复现步骤和必要的日志片段，并先移除账号、令牌、路径和其他敏感信息。

## 许可证

当前仓库尚未声明开源许可证。在许可证文件发布前，源码的复制、修改与再分发权限不作默认授权。
