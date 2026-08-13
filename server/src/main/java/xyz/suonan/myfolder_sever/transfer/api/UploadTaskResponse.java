package xyz.suonan.myfolder_sever.transfer.api;

import xyz.suonan.myfolder_sever.transfer.model.UploadFile;
import xyz.suonan.myfolder_sever.transfer.model.UploadState;
import xyz.suonan.myfolder_sever.transfer.model.UploadTask;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public record UploadTaskResponse(
        String uploadId,
        String targetPath,
        UploadState state,
        int chunkSize,
        int totalFiles,
        long totalBytes,
        Instant createdAt,
        Instant updatedAt,
        List<FileStatus> files
) {
    public record FileStatus(
            String path,
            long size,
            String sha256,
            int totalChunks,
            UploadState state,
            List<Integer> completedChunks,
            List<Integer> missingChunks,
            String failureReason
    ) {
    }

    public static UploadTaskResponse from(UploadTask task) {
        List<FileStatus> statuses = task.files.values().stream().map(UploadTaskResponse::fileStatus).toList();
        return new UploadTaskResponse(task.uploadId, task.targetPath, task.state, task.chunkSize, task.totalFiles,
                task.totalBytes, task.createdAt, task.updatedAt, statuses);
    }

    private static FileStatus fileStatus(UploadFile file) {
        List<Integer> completed = file.completedChunks.stream().sorted().toList();
        List<Integer> missing = new ArrayList<>();
        if (file.state != UploadState.COMPLETED) {
            for (int index = 0; index < file.totalChunks; index++) {
                if (!file.completedChunks.contains(index)) {
                    missing.add(index);
                }
            }
        }
        missing.sort(Comparator.naturalOrder());
        return new FileStatus(file.path, file.size, file.sha256, file.totalChunks, file.state,
                completed, missing, file.failureReason);
    }
}
