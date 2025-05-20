package xyz.suonan.myfolder_sever.Utils;

import xyz.suonan.myfolder_sever.MyObject.Directory;
import xyz.suonan.myfolder_sever.MyObject.FileBaseItem;
import xyz.suonan.myfolder_sever.MyObject.MyFile;

import java.io.File;
import java.nio.file.Path;
import java.util.Date;

public class WrapFileBaseItem {

    private WrapFileBaseItem() {}
    public static FileBaseItem wrapFileBaseItem(Path ItemPath){
        File file = ItemPath.toFile();
        if(!file.exists()){
            return null;
        }
        else if(file.isDirectory()){
            return new Directory(file.getName(),"Directory",ItemPath);
        }
        else if(file.isFile()){
            return new MyFile("File",file.getName(),file.length(),new Date(file.lastModified()),ItemPath);
        }
        else{
            return null;
        }
    }
}
