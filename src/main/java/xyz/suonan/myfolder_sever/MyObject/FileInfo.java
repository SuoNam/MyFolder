package xyz.suonan.myfolder_sever.MyObject;

public class FileInfo implements FileBase{
    public String path;
    public long size;
    public long mtime;
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
