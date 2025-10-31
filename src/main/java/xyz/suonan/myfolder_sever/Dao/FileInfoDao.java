package xyz.suonan.myfolder_sever.Dao;

import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Select;
import xyz.suonan.myfolder_sever.MyObject.FileInfo;

import java.util.List;

public interface FileInfoDao {

    @Select("select path from file_info where sha256=#{sha256}")
    List<String> select(byte[] sha256);

    @Insert("""
    insert into file_info(path,sha256,size,mtime) values(#{path},#{sha256},#{size},#{mtime})
""")
    int insert(FileInfo fileInfo);


    @Delete("""
    delete from file_info where path=#{path}
""")
    void deleteByPath(String path);
}