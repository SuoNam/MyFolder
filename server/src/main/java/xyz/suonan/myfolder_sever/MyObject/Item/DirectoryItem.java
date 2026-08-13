package xyz.suonan.myfolder_sever.MyObject.Item;

import java.nio.file.Path;
import java.util.Date;
import java.util.List;
public class DirectoryItem implements FileBaseItem {

    private String type;
    private String directoryName;
    private List<FileInfoItem> fileList;
    private Path path;
    public Date lastModified;
    public DirectoryItem(String directoryName, String type, List<FileInfoItem> files, Date lastModified) {
        this.directoryName = directoryName;
        fileList=files;
        this.type = type;
        this.lastModified = lastModified;
    }
    public DirectoryItem(String directoryName, String type, Path path, Date lastModified) {
        this.directoryName = directoryName;
        this.type = type;
        this.path = path;
        this.lastModified = lastModified;
    }

    public String toString(){
        return type+"|"+directoryName;
    }

    @Override
    public String getPath() {
        return path.toString();
    }
    @Override
    public String getType(){
        return type;
    }
    @Override
    public String getName(){
        return directoryName;
    }

}
