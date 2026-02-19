package xyz.suonan.myfolder_sever.MyObject;

import lombok.Getter;
import lombok.Setter;

public class FileInfoResponse implements FileBase{
    public String path;
    public byte[] sha256;
    @Setter
    @Getter
    public boolean exists;

    public FileInfoResponse(String path, byte[] sha256) {
        this.path = path;
        this.sha256 = sha256;
    }

    @Override
    public byte[] getSha256() {
        return sha256;
    }

    @Override
    public String getPath() {
        return path;
    }




}
