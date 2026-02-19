# MyFolder 
MyFolder 是一款基于 Spring Boot 和 Vue 开发的轻量级文件管理系统。

## ✨ 核心功能
文件操作：支持批量文件上传、下载、删除功能。

文件夹操作：支持文件夹的上传、下载、删除功能，针对文件夹中相同内容的文件支持秒传功能

目录管理：支持创建空文件夹、文件/文件夹的复制与粘贴。

权限分级：

游客模式：仅能浏览和下载文件。

登录模式：必须登录后方可对目标文件夹进行更改（上传、复制、删除等操作）。

## 🛠️ 技术栈
后端：Spring Boot, JWT (安全验证)

前端：Vue.js

数据存储：MySQL (元数据)、Redis (Session/缓存)

部署环境：支持 Debian/Ubuntu (.deb 安装包)

## 🚀 快速开始
1. **环境准备**
   
   确保你的服务器已安装以下组件：
   - MySQL 8.0+
   - Redis

2. **获取与安装**
   
   从 [releases](https://github.com/SuoNam/MyFolder/releases)页面下载最新的 .deb 安装包并执行：
   ```bash
   sudo dpkg -i myfolder_v0.1.0-beta_amd64.deb
   ```

3. **初始化**

   安装后会弹出初始化脚本，请根据指引输入 MySQL root 密码。

5. **初始用户**

   当前版本暂未开放注册接口，需在数据库手动插入初始用户
