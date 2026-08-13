# PC 客户端上传接口迁移说明（v1.1.1）

## 结论

准确接口名是 `/file/uploadfile`，不是 `/file/upload`。

原 `/file/uploadfile` 是一次性 `multipart/form-data` 整文件上传；v1.1.1 已改成“创建任务 → 查询断点 → 上传原始分片 → 逐文件校验完成 → 整任务完成”的协议，支持分片 SHA-256、整文件 SHA-256 和断点续传。

- 正式后端：`https://api.myfolder.com.cn`
- 测试后端：`https://test.myfolder.com.cn`
- 所有上传请求都要带裸 JWT：`Authorization: <jwt>`，不要添加 `Bearer `。

## 新老差异

| 项目 | 旧版 | v1.1.1 新版 |
|---|---|---|
| 创建接口 | `POST /file/uploadfile` | `POST /file/uploadfile` |
| Content-Type | `multipart/form-data` | `application/json` |
| 请求字段 | `files`（一个或多个整文件）、`path` | `targetPath`、`chunkSize`、`totalFiles`、`totalBytes`、`files[]` 清单 |
| 文件内容 | 创建请求中一次传完整文件 | 创建后通过 `PUT .../chunks/{chunkIndex}` 逐块传原始字节 |
| 完整性校验 | 写盘后由服务器计算一次文件哈希，客户端不声明哈希 | 客户端声明每个文件 SHA-256；每块也必须带 SHA-256；服务端逐块和整文件分别校验 |
| 断点续传 | 不支持 | `GET /file/uploadfile/{uploadId}` 获取 `missingChunks`，只补缺块 |
| 落盘位置 | `basePath / path / 原文件名` | `storageRoot / targetPath / files[].path` |
| 收尾 | 单次请求结束即结束 | 每个文件必须调用 `/files/complete`，全部文件完成后再调用任务 `/complete` |
| 返回结构 | `BaseMessage` 包装，内部是每个文件的成功/失败结果 | 直接返回 `UploadTaskResponse`，不是 `BaseMessage` |
| HTTP 状态 | 外层通常 HTTP 200，业务状态写在 JSON 中 | 创建成功 201；其余成功 200；参数、校验和状态冲突使用真实 4xx/5xx |
| 暂存与移动 | 直接写最终目录；Web 曾需要上传后再 `/file/move` | 分片先写 `.myfolder/staging`；整文件校验成功后由服务端直接落到目标目录，不再调用 `/file/move` 或 `/file/delete` 清理暂存目录 |

## 新协议完整流程

### 1. 客户端预计算清单

对每个本地文件计算：

- `size`：文件字节数。
- `sha256`：整个文件的 SHA-256，64 位十六进制字符串。
- `totalChunks = ceil(size / chunkSize)`；空文件为 `0`。
- `path`：文件在本次上传目录内的相对路径，例如 `docs/a.txt`。

`chunkSize` 必须在 65536（64 KiB）到 67108864（64 MiB）之间。建议 PC 与 Web 一致使用 4194304（4 MiB）。

### 2. 创建任务

```http
POST /file/uploadfile
Authorization: <jwt>
Content-Type: application/json
```

```json
{
  "targetPath": "inbox/demo",
  "chunkSize": 4194304,
  "totalFiles": 1,
  "totalBytes": 5,
  "files": [
    {
      "path": "docs/a.txt",
      "size": 5,
      "sha256": "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824",
      "totalChunks": 1
    }
  ]
}
```

成功返回 HTTP 201 和任务对象：

```json
{
  "uploadId": "...",
  "targetPath": "inbox/demo",
  "state": "PENDING",
  "chunkSize": 4194304,
  "totalFiles": 1,
  "totalBytes": 5,
  "createdAt": "...",
  "updatedAt": "...",
  "files": [
    {
      "path": "docs/a.txt",
      "size": 5,
      "sha256": "...",
      "totalChunks": 1,
      "state": "PENDING",
      "completedChunks": [],
      "missingChunks": [0],
      "failureReason": null
    }
  ]
}
```

最终文件位置是 `targetPath + "/" + files[i].path`。`targetPath` 可为 `""`，表示服务器文件根目录，不需要为了根目录上传建立临时目录。

### 3. 上传分片

```http
PUT /file/uploadfile/{uploadId}/chunks/{chunkIndex}
Authorization: <jwt>
Content-Type: application/octet-stream
X-File-Path: docs%2Fa.txt
X-Chunk-SHA256: <当前分片的 SHA-256>
Content-Range: bytes 0-4/5

<5 个原始文件字节>
```

规则：

- `chunkIndex` 对每个文件分别从 `0` 开始。
- `X-File-Path` 是清单中 `files[].path` 的 UTF-8 URL 编码值，服务端解码一次。
- `Content-Range` 必须严格是 `bytes start-end/文件总大小`，`end` 为包含式下标。
- 普通分片：`start = chunkIndex * chunkSize`，`end = min(size, start + chunkSize) - 1`。
- 请求体只能是该分片的原始字节，不能使用 multipart，也不能 Base64。
- 每块上传成功后返回完整任务对象。客户端应以服务器返回的 `completedChunks` / `missingChunks` 为准。

### 4. 查询并恢复断点

```http
GET /file/uploadfile/{uploadId}
Authorization: <jwt>
```

恢复前先核对用户重新选择的本地文件与服务器清单中的 `path`、`size` 和整文件 `sha256` 是否相同，然后只上传服务器返回的 `missingChunks`。

不要只依赖本地进度；任务状态和缺块列表以服务器为准。

### 5. 完成每个文件

每个非空文件的所有分片成功后都必须调用：

```http
POST /file/uploadfile/{uploadId}/files/complete
Authorization: <jwt>
Content-Type: application/json

{"filePath":"docs/a.txt"}
```

服务端会检查缺块、实际文件大小以及整个文件 SHA-256，成功后才把暂存文件落到最终目录，并把该文件状态设为 `COMPLETED`。

注意：`missingChunks: []` 只表示分片齐全，不表示整文件已校验完成。跳过本步骤直接调用任务 `/complete`，会得到 `409 FILE_INCOMPLETE`。

空文件在创建任务时已经校验空文件 SHA-256 并创建，不需要上传分片。

### 6. 完成整个任务

```http
POST /file/uploadfile/{uploadId}/complete
Authorization: <jwt>
```

只有全部文件都为 `COMPLETED` 时才成功。成功返回任务 `state: "COMPLETED"`。该 `uploadId` 之后可以直接用于 RELAY 转发的 `relayUploadId`。

### 7. 取消任务

```http
DELETE /file/uploadfile/{uploadId}
Authorization: <jwt>
```

返回任务 `state: "CANCELLED"`。已经 `COMPLETED` 的任务不能取消。

## 路径规则

`targetPath` 允许空字符串；`files[].path` 必须是非空安全相对路径。路径统一使用 `/`：

- 不能以 `/` 开头或结尾。
- 不能包含反斜杠 `\\`、盘符路径、空路径段、`.` 或 `..`。
- 不能利用符号链接逃出存储根目录。
- 客户端不要把文件名拼到 `targetPath` 两次。比如目标目录 `inbox`、文件 `a.txt` 应传 `targetPath: "inbox"`、`path: "a.txt"`。

## 错误响应

新版错误不再使用旧 `BaseMessage`，格式如下：

```json
{
  "timestamp": "...",
  "status": 409,
  "code": "FILE_INCOMPLETE",
  "message": "File still has missing chunks",
  "path": "/file/uploadfile/.../files/complete",
  "details": {"missingChunks": [2, 3]}
}
```

PC 端应优先根据 `HTTP status + code` 处理，不要匹配英文 `message`。主要错误码：

- `400 INVALID_REQUEST`：清单字段、数量、大小或 SHA-256 格式错误。
- `400 INVALID_PATH`：不安全路径。
- `400 CHUNK_INDEX_INVALID`：分片编号越界。
- `400 CHUNK_RANGE_INVALID`：`Content-Range` 与清单不一致。
- `400 CHUNK_SIZE_INVALID`：请求体长度与范围不一致。
- `404 TASK_NOT_FOUND`：任务不存在，或任务不属于当前账号。
- `404 FILE_NOT_FOUND`：文件不在任务清单中。
- `409 FILE_INCOMPLETE`：仍有缺块，或有文件尚未完成整文件校验。
- `409 TASK_STATE_CONFLICT`：任务已完成/取消，当前操作不允许。
- `409 TARGET_ALREADY_EXISTS`：目标已有不同内容的同名文件；服务端不会静默覆盖。
- `422 CHUNK_HASH_MISMATCH`：当前分片 SHA-256 不一致，重读本地分片后重传。
- `422 FILE_SIZE_MISMATCH` / `FILE_HASH_MISMATCH`：整文件校验失败；服务端会清空该文件的已完成分片状态，需要重新上传该文件。
- `500 STORAGE_ERROR`：服务端存储读写失败。

## PC 端哪些代码需要改

如果 PC 当前调用旧的 multipart `POST /file/uploadfile`，必须修改：

1. 删除 multipart 上传实现。
2. 增加流式计算整文件 SHA-256，避免一次把大文件读入内存。
3. 增加创建任务和保存 `uploadId`。
4. 按分片读取文件、计算当前分片 SHA-256，并发送原始字节。
5. 支持通过 GET 状态读取 `missingChunks` 后恢复上传。
6. 每个文件上传完调用一次 `/files/complete`。
7. 全部文件完成后调用任务 `/complete`。
8. 用 HTTP 状态码和稳定 `code` 解析错误。
9. 上传成功后不要再调用 `/file/move` 和 `/file/delete` 处理临时目录。

如果 PC 的文件中转已经使用 `/api/v1/transfers/tasks/*`，路由暂时不用切换；该兼容入口仍接受：

```json
{
  "parentPath": "relay",
  "directoryName": "demo",
  "chunkSize": 4194304,
  "totalFiles": 1,
  "totalBytes": 5,
  "files": [
    {"path":"docs/a.txt","size":5,"sha256":"...","totalChunks":1}
  ]
}
```

服务端会把它转换为 `targetPath = parentPath + "/" + directoryName`。其分片上传、查询状态、逐文件完成、任务完成和取消路径仍是 `/api/v1/transfers/tasks/{uploadId}/*`。尤其要确认 PC 已调用 `/files/complete`；否则任务不能完成，也不能作为 RELAY 源。

旧 `/directory/*` 上传接口目前仅作为旧客户端兼容入口，和本次 `/file/uploadfile` 迁移不是同一个协议；新代码不要再基于它开发。

## 联调入口

- 正式 Swagger：`https://api.myfolder.com.cn/swagger-ui.html`
- 正式 OpenAPI：`https://api.myfolder.com.cn/v3/api-docs`
- 测试 Swagger：`https://test.myfolder.com.cn/swagger-ui.html`
- 测试 OpenAPI：`https://test.myfolder.com.cn/v3/api-docs`

