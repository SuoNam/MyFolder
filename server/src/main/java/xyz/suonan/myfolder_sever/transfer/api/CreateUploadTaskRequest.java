package xyz.suonan.myfolder_sever.transfer.api;

import java.util.List;

public record CreateUploadTaskRequest(
        String scopeType,
        String scopeId,
        String targetPath,
        int chunkSize,
        int totalFiles,
        long totalBytes,
        List<FileManifest> files
) {
    public CreateUploadTaskRequest(String targetPath, int chunkSize, int totalFiles, long totalBytes,
                                   List<FileManifest> files) {
        this("PRIVATE", null, targetPath, chunkSize, totalFiles, totalBytes, files);
    }
    public record FileManifest(String path, long size, String sha256, int totalChunks) {
    }
}
