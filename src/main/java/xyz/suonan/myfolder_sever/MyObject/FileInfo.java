package xyz.suonan.myfolder_sever.MyObject;

public class FileInfo implements FileBase{
    public String path;
    public long size;
    public long mtime;
    public String sha256;

    public FileInfo(String path, long size, long mtime, String sha256) {
        this.path = path;
        this.size = size;
        this.mtime = mtime;
        this.sha256 = sha256;
    }

    @Override
    public String getPath() {
        return "";
    }

    @Override
    public String getSha256() {
        return "";
    }
}
