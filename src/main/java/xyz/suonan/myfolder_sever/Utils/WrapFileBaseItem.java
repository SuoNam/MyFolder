package xyz.suonan.myfolder_sever.Utils;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import xyz.suonan.myfolder_sever.MyObject.Item.DirectoryItem;
import xyz.suonan.myfolder_sever.MyObject.Item.FileBaseItem;
import xyz.suonan.myfolder_sever.MyObject.Item.FileInfoItem;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Date;
@Component
public class WrapFileBaseItem {

    private final Path basePath;

    @Autowired
    public WrapFileBaseItem(@Value("${basePath}") String basePathStr) {
        this.basePath = Paths.get(basePathStr);
    }

    public FileBaseItem wrapFileBaseItem(Path ItemPath){
        File file = ItemPath.toFile();
        if(!file.exists()){
            return null;
        }
        else if(file.isDirectory()){
            return new DirectoryItem(file.getName(),"Directory",this.basePath.relativize(ItemPath),new Date(file.lastModified()));
        }
        else if(file.isFile()){
            return new FileInfoItem("File",file.getName(),file.length(),new Date(file.lastModified()),this.basePath.relativize(ItemPath));
        }
        else{
            return null;
        }
    }
}
