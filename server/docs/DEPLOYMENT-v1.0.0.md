# MyFolder Server v1.1.1 部署安全说明

## 必需环境变量

生产环境启动前必须从部署平台的 Secret Store 注入：

```text
MYFOLDER_PROFILE=formal
MYFOLDER_JWT_SECRET=<至少 32 字节随机值>
MYFOLDER_DB_URL=<数据库 JDBC URL>
MYFOLDER_DB_USERNAME=<数据库用户>
MYFOLDER_DB_PASSWORD=<数据库密码>
MYFOLDER_REDIS_HOST=<Redis 主机>
MYFOLDER_REDIS_PASSWORD=<Redis 密码>
MYFOLDER_STORAGE_ROOT=<文件存储绝对路径>
MYFOLDER_DEVICE_METADATA_FILE=<设备元数据持久化文件>
MYFOLDER_FORWARD_METADATA_FILE=<转发任务元数据持久化文件>
```

可选变量和本地占位模板见 `server/.env.example`。`.env` 文件已被 Git 忽略；Spring Boot 不会自动读取 `.env`，部署器需要把它们注入进程环境。

代码仓库曾经出现过明文数据库、Redis 和 JWT 凭据。删除配置中的值不能使旧凭据重新安全，部署前必须在对应系统中全部轮换。

## HTTPS

生产环境只允许客户端访问 HTTPS/WSS。`formal` profile 默认启用 Spring Boot 原生 TLS；未提供有效 keystore 时，生产实例不得启动并对公网提供明文服务。

### 方案 A：Spring Boot 原生 TLS（formal 默认）

部署 Secret Store 必须提供：

```text
MYFOLDER_TLS_ENABLED=true
MYFOLDER_TLS_KEY_STORE=file:/etc/myfolder/tls/myfolder.p12
MYFOLDER_TLS_KEY_STORE_PASSWORD=<secret>
MYFOLDER_TLS_KEY_STORE_TYPE=PKCS12
MYFOLDER_TLS_KEY_ALIAS=myfolder
```

证书的 SAN 必须覆盖客户端使用的生产域名；不要以裸 IP 作为生产默认地址，除非证书明确包含该 IP。HTTPS REST 与 `/device` WSS 共用同一个 TLS 监听端口。

### 方案 B：反向代理终止 TLS

也可在 Nginx、Caddy、Traefik 或云负载均衡器终止 TLS：

```text
PC Client -- HTTPS --> TLS Proxy -- private HTTP --> MyFolder Server:8082
```

`application-formal.properties` 已启用 `server.forward-headers-strategy=framework`，代理必须覆盖并传递可信的 `Forwarded` 或 `X-Forwarded-*` 头。应用端口不得直接暴露到公网。

代理方案必须显式设置：

```text
MYFOLDER_TLS_ENABLED=false
MYFOLDER_SERVER_ADDRESS=127.0.0.1
```

若代理与应用不在同一主机，`MYFOLDER_SERVER_ADDRESS` 必须绑定到受防火墙保护的私网地址。代理还必须正确转发 WebSocket 的 Upgrade/Connection 头。

证书、私钥和 keystore 密码禁止提交到仓库。每次部署必须实际验证，不能只检查配置文件：

```bash
openssl s_client -connect myfolder.example.com:443 -servername myfolder.example.com -verify_return_error </dev/null
curl --fail --show-error --head https://myfolder.example.com/
websocat -H='Authorization: <jwt>' -H='X-Device-Token: <device-token>' 'wss://myfolder.example.com/device?deviceId=<device-id>'
```

验收要求：证书链校验成功、主机名匹配、仅协商 TLS 1.2/1.3，且 WSS 完成 HTTP 101 Upgrade。客户端的 REST Base URL 与 WebSocket URL 必须指向同一已验收的生产域名。

## 文件存储

- 存储根目录必须由专用服务账户拥有；
- 禁止其他用户在根目录内创建符号链接；
- 服务端还会在每次解析目标路径时拒绝已存在的符号链接路径段；
- `.myfolder/staging` 和任务元数据目录应位于持久磁盘；
- 设备和转发元数据文件所在目录必须位于持久磁盘，并限制为服务账户可读写；
- 定期备份已完成文件和任务快照。
