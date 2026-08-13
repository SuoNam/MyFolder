package xyz.suonan.myfolder_sever.MyObject;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Data;
import xyz.suonan.myfolder_sever.Deserializer.HexToByteArrayDeserializer;

@Data
public class FileInfo implements FileBase{
    public String path;
    public long size;
    public long mtime;
    @JsonDeserialize(using = HexToByteArrayDeserializer.class)
    public byte[] sha256;

    public FileInfo(String path, long size, long mtime, byte[] sha256) {
        this.path = path;
        this.size = size;
        this.mtime = mtime;
        this.sha256 = sha256;
    }

    @Override
    public String getPath() {
        return path;
    }

    @Override
    public byte[] getSha256() {
        return sha256;
    }
}
