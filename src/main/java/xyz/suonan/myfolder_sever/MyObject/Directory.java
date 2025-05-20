package xyz.suonan.myfolder_sever.MyObject;
import java.nio.file.Path;
import java.util.List;
public class Directory implements FileBaseItem {

    private String type;
    private String directoryName;
    private List<MyFile> fileList;
    private Path path;
    public Directory(String directoryName,String type,List<MyFile> files) {
        this.directoryName = directoryName;
        fileList=files;
        this.type = type;
    }
    public Directory(String directoryName, String type, Path path) {
        this.directoryName = directoryName;
        this.type = type;
        this.path = path;
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
