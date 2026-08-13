# MyFolder Web

MyFolder Web 是 MyFolder 的浏览器端控制台，使用 Vue 3、TypeScript、Vite、Pinia 和 Vue Router 构建。

> 当前版本：**v1.1.1**  
> 官网：<https://myfolder.com.cn>  
> Web 控制台：<https://web.myfolder.com.cn>  
> API：<https://api.myfolder.com.cn>

## 功能

- 账号注册、登录、退出和会话刷新
- Nyauth、Google、GitHub OAuth 登录
- OAuth 登录回调处理
- OAuth 账号绑定与解绑
- 邮箱验证码与敏感操作二次验证
- 设备列表、设备认领和设备管理
- 文件浏览、上传、下载和发送
- 传输任务、历史记录和存储用量查看
- 账号资料与安全设置

## 分支说明

- [`main`](https://github.com/SuoNam/MyFolder/tree/main)：服务端源码、部署文档和 Web v1.1.1 生产构建产物
- [`web`](https://github.com/SuoNam/MyFolder/tree/web)：Web 前端源码
- [`client`](https://github.com/SuoNam/MyFolder/tree/client)：Qt Windows 客户端源码

## 技术栈

- Vue 3
- TypeScript
- Vite 6
- Pinia
- Vue Router
- Axios

## 本地开发

需要 Node.js 20 或更高版本以及 npm。

```bash
npm ci
copy .env.example .env.local
npm run dev
```

默认 API 地址为：

```text
https://api.myfolder.com.cn
```

也可以通过环境变量覆盖：

```env
VITE_API_BASE=https://api.example.com
```

## 类型检查与构建

```bash
npm run typecheck
npm run build
```

生产构建产物会生成到 `dist/`。`dist/` 不在本分支追踪；正式生产产物同时收录在 `main` 分支的 `web/` 目录中。

## SPA 部署

Web 使用 History 路由。Nginx 等静态服务器必须将不存在的路径回退到 `index.html`，否则直接访问 `/oauth/callback`、`/files` 等路由会返回 404。

Nginx 示例：

```nginx
server {
    server_name web.myfolder.com.cn;
    root /www/wwwroot/web.myfolder.com.cn;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }
}
```

## OAuth 回调

浏览器端 OAuth 回调路由为：

```text
https://web.myfolder.com.cn/oauth/callback
```

OAuth 提供商和服务端必须使用一致的 HTTPS 回调域名。不要将 OAuth Client Secret 放入 Web 源码或任何 `VITE_` 环境变量中；Vite 环境变量会被编译进浏览器端产物。

## 安全说明

- 不要提交密码、JWT 密钥、OAuth Client Secret、邮件服务密钥或 TLS 私钥。
- 浏览器端只能保存公开配置，敏感凭据必须由服务端保管。
- 生产环境应使用 HTTPS，并在服务端限制可信 CORS 来源。

## 相关链接

- 官网：<https://myfolder.com.cn>
- Web 控制台：<https://web.myfolder.com.cn>
- API：<https://api.myfolder.com.cn>
- 仓库：<https://github.com/SuoNam/MyFolder>
