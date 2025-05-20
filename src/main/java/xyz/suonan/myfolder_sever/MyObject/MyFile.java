package xyz.suonan.myfolder_sever.myObject;

import lombok.Data;
import lombok.Getter;

import java.util.Date;
@Data
public class File implements FileBaseItem{
  @Getter
  public String type;
  public String fileName;
  public long size;
  public Date lastModified;


    public String getName(){
    return fileName;
  }
}
