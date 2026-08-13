# `/file/uploadfile` 分块上传协议

旧的 `multipart/form-data` 整文件上传已经删除。Web 使用 `/file/uploadfile`，提供逐块 SHA-256、整文件 SHA-256 和断点续传。

桌面客户端的 RELAY 中转仍可使用 `/api/v1/transfers/tasks`，该兼容入口继续接受 `parentPath + directoryName`；其余分块、文件收尾和任务收尾路径保持不变。新 Web 不使用这个兼容入口。

所有请求都需要裸 JWT，不加 `Bearer`：

```http
Authorization: <jwt>
```

## 1. 创建上传任务

```http
POST /file/uploadfile
Content-Type: application/json
```

```json
{
  "targetPath": "inbox/demo",
  "chunkSize": 65536,
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

最终位置为 `storageRoot / targetPath / files[].path`。`targetPath` 可以是空字符串，表示服务器文件根目录；`files[].path` 必须是非空安全相对路径。

`chunkSize` 必须介于 64 KiB 和 64 MiB。服务端返回 `uploadId`、`targetPath`、`completedChunks` 和 `missingChunks`。

## 2. 查询断点

```http
GET /file/uploadfile/{uploadId}
```

前端重新选择文件后，应先确认本地文件的大小和完整 SHA-256 与任务清单一致，再只上传 `missingChunks`。

## 3. 上传一个分块

```http
PUT /file/uploadfile/{uploadId}/chunks/{chunkIndex}
Content-Type: application/octet-stream
X-File-Path: docs%2Fa.txt
X-Chunk-SHA256: <当前分块 SHA-256>
Content-Range: bytes 0-4/5
```

请求体是原始分块字节。服务端在写盘前验证分块 SHA-256，不一致返回 `422 CHUNK_HASH_MISMATCH`。

## 4. 完成单个文件

```http
POST /file/uploadfile/{uploadId}/files/complete
Content-Type: application/json

{"filePath":"docs/a.txt"}
```

服务端确认没有缺块后，对整个暂存文件重新计算 SHA-256。验证成功后直接落到 `targetPath`，不需要再次调用 `/file/move`。

## 5. 完成或取消任务

```http
POST /file/uploadfile/{uploadId}/complete
DELETE /file/uploadfile/{uploadId}
```

只有所有文件完成整文件校验后，任务才会进入 `COMPLETED`。完成任务得到的 `uploadId` 可直接用于 forwards 的 RELAY 流程。
