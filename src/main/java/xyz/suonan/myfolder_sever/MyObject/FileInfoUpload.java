package xyz.suonan.myfolder_sever.MyObject;

import lombok.Getter;
import lombok.Setter;


public class FileInfoUpload extends FileInfo {
    public int totalChunks;
    public FileInfoUpload(String path, byte[] sha256, long size, long mtime,int totalChunks) {
        super(path, size, mtime,sha256);
        this.totalChunks = totalChunks;
    }

}
