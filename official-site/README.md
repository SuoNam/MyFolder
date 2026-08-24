# MyFolder Official Website

MyFolder 官方网站，基于 Vue 3 与 Vite 构建，展示 v1.1.1 的多设备文件流转、LAN / P2P / Server Relay 智能路由、可靠传输与 OAuth 账号认证能力。

## 技术栈

- Vue 3
- Vite 6
- 原生 CSS（响应式布局、深浅主题、进入动画）

## 本地开发

```bash
npm install
npm run dev
```

## 生产构建

```bash
npm run build
npm run preview
```

构建产物位于 `dist/`。

## 服务器部署

线上站点目录：

```text
/www/wwwroot/myfolder.com.cn
```

部署时应将 `dist/` **目录内的文件**复制到站点根目录，使服务器最终结构类似：

```text
/www/wwwroot/myfolder.com.cn/index.html
/www/wwwroot/myfolder.com.cn/assets/...
```

覆盖线上目录前请先备份，并验证官网、移动端布局、主题切换和外部链接。

## 相关地址

- 官网：<https://myfolder.com.cn>
- Web 控制台：<https://web.myfolder.com.cn>
- API：<https://api.myfolder.com.cn>
- GitHub：<https://github.com/SuoNam/MyFolder>

## 分支约定

- `main`：服务端源码与 Web 构建产物
- `web`：Web 前端源码
- `client`：Windows 桌面客户端源码
