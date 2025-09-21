package xyz.suonan.myfolder_sever.MyObject;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class FileInfoUpload extends FileInfo {
    public int totalChunks;
    public FileInfoUpload(String path, String sha256, long size, long mtime,int totalChunks) {
        super(path, size, mtime,sha256);
        this.totalChunks = totalChunks;
    }

}
