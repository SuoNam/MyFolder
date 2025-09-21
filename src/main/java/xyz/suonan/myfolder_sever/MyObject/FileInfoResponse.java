package xyz.suonan.myfolder_sever.MyObject;

import lombok.Getter;

public class FileInfoResponse implements FileBase{
    public String path;
    public String sha256;
    @Getter
    public boolean exists;
    @Override
    public String getSha256() {
        return sha256;
    }

    @Override
    public String getPath() {
        return sha256;
    }
}
