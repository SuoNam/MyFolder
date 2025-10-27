package xyz.suonan.myfolder_sever.Dao;

import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Select;
import xyz.suonan.myfolder_sever.pojo.DirectoryInfo;

import java.util.List;

public interface DirectoryInfoDao {
    @Insert("""
        INSERT INTO directory_info
        (id, directory_name, total_files, chunk_size, total_bytes,
         parent_path,status, creation_date)
        VALUES
        (#{id}, #{directoryName}, #{totalFiles}, #{chunkSize}, #{totalBytes},
         #{parentPath}, #{status}, #{creationDate})
        """)
    int insert(DirectoryInfo directoryInfo);


    @Select("""
        select  id  from directory_info where id=#{uuid}
""")
    List<String> selectAll(String uuid);

    @Select("""
            select directory_name from directory_info where id=#{uuid}
""")
    List<String> selectNameByID(String uuid);

    @Delete("""
        delete from  directory_info where id=#{uuid}
""")
    int deleteByID(String uuid);
}
