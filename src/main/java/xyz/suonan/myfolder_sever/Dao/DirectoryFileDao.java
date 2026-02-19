package xyz.suonan.myfolder_sever.Dao;

import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Insert;

public interface DirectoryFileDao {

    @Insert("""
    insert into directory_file(id,relative_path) values(#{uploadId},#{relativePath})
""")
    void insertByIdAndRelativePath(String uploadId,String relativePath);



    @Delete("""
    delete from directory_file where id=#{uploadId}
""")
    void deleteById(String uploadId);
}
