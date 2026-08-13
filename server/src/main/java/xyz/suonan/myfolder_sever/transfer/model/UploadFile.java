package xyz.suonan.myfolder_sever.transfer.model;

import java.util.LinkedHashSet;
import java.util.Set;

public class UploadFile {
    public String path;
    public long size;
    public String sha256;
    public int totalChunks;
    public Set<Integer> completedChunks = new LinkedHashSet<>();
    public UploadState state = UploadState.PENDING;
    public String failureReason;
    public boolean materializedByTask;

    public UploadFile() {
    }

    public UploadFile(String path, long size, String sha256, int totalChunks) {
        this.path = path;
        this.size = size;
        this.sha256 = sha256;
        this.totalChunks = totalChunks;
    }
}
