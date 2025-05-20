package xyz.suonan.myfolder_sever.MyObject;
import java.nio.file.Path;
import java.util.Date;
public class MyFile implements FileBaseItem{

  private String type;
  private String fileName;
  public long size;
  public Date lastModified;
  private Path  path;
  public MyFile(String type, String fileName, long size, Date lastModified,Path path) {
    this.type = type;
    this.fileName = fileName;
    this.size = size;
    this.lastModified = lastModified;
    this.path = path;
  }
  @Override
  public String getType(){
    return type;
  }
  @Override
    public String getName(){
    return fileName;
  }
  @Override
  public String toString(){
    return type+"|"+fileName+"|"+size+"|"+lastModified;
  }
  @Override
  public String getPath(){
    return path.toString();
  }
}
