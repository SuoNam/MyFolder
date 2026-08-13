package xyz.suonan.myfolder_sever.transfer.model;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.time.Instant;
import java.util.LinkedHashMap;
import java.util.Map;

public class UploadTask {
    public String uploadId;
    public String ownerUserId;
    public String scopeType;
    public String scopeId;
    public String targetPath;
    /** Read-only migration fields for tasks persisted by the previous path model. */
    @JsonProperty(access = JsonProperty.Access.WRITE_ONLY)
    public String directoryName;
    @JsonProperty(access = JsonProperty.Access.WRITE_ONLY)
    public String parentPath;
    public int totalFiles;
    public long totalBytes;
    public int chunkSize;
    public UploadState state = UploadState.PENDING;
    public Instant createdAt;
    public Instant updatedAt;
    public String failureReason;
    public Map<String, UploadFile> files = new LinkedHashMap<>();

    public UploadTask() {
    }
}
