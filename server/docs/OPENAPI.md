# MyFolder OpenAPI / 前端联调

服务启动后可使用：

- Swagger UI：`https://<host>/swagger-ui.html`
- OpenAPI JSON：`https://<host>/v3/api-docs`
- OpenAPI YAML：`https://<host>/v3/api-docs.yaml`

## 鉴权

注册、登录和公开文件读取不需要鉴权。其他接口在 Swagger UI 右上角点击 **Authorize**，直接输入 JWT；不要添加 `Bearer`。

```http
Authorization: eyJhbGciOiJIUzI1NiJ9...
```

以下读取接口刻意公开，但路径仍被限制在 `basePath` 内：

- `POST /file/getfilelist`
- `GET /file/downloadfile`
- `GET /directory/downloaddirectory`

以下旧文件修改接口均要求 JWT：

- `POST /file/uploadfile`
- `POST /file/createfolder`
- `POST /file/move`
- `POST /file/delete`
- `POST /file/copy`

## 可恢复上传顺序

新前端应使用 `/file/uploadfile`，并严格按以下顺序：

1. `POST /file/uploadfile` 创建任务，使用 `targetPath` 指定目标目录，空字符串表示根目录。
2. `PUT /file/uploadfile/{uploadId}/chunks/{chunkIndex}` 上传所有分片。
3. 对每个文件调用 `POST /file/uploadfile/{uploadId}/files/complete`。
4. 调用 `POST /file/uploadfile/{uploadId}/complete` 完成整个任务。
5. RELAY 发送再使用已完成的 `uploadId` 创建 `/api/v1/forwards`。

`missingChunks: []` 只说明分片齐全，不代表文件已经完成 SHA-256 校验。

## 前端生成类型

可将 `/v3/api-docs` 输入 `openapi-typescript`、Orval 或 OpenAPI Generator。例如：

```bash
npx openapi-typescript https://test.myfolder.com.cn/v3/api-docs -o src/api/schema.d.ts
```

旧 `/directory/*` 上传接口仅用于兼容旧客户端。桌面客户端的 RELAY 中转继续使用 `/api/v1/transfers/tasks/*`，Web 文件上传使用 `/file/uploadfile/*`。
