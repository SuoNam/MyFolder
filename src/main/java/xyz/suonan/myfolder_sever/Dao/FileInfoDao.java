package xyz.suonan.myfolder_sever.Dao;

import org.apache.ibatis.annotations.*;
import xyz.suonan.myfolder_sever.MyObject.FileInfo;

import java.util.List;

public interface FileInfoDao {

    @Select("select path from file_info where sha256=#{sha256}")
    List<String> selectBySha256(byte[] sha256);

    @Insert("""
    insert into file_info(path,sha256,size,mtime) values(#{path},#{sha256},#{size},#{mtime})
""")
    int insert(FileInfo fileInfo);
    @Update("""
    update file_info set path=#{newPath} where path=#{oldPath}
""")
    int updatePathByPath(@Param("oldPath") String oldPath,@Param("newPath")String newPath);
    @Delete("""
    delete from file_info where path=#{path}
""")
    void deleteByPath(String path);
}