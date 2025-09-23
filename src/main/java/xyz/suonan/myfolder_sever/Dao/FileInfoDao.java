package xyz.suonan.myfolder_sever.Dao;

import org.apache.ibatis.annotations.Select;

import java.util.List;

public interface FileInfoDao {

    @Select("select path from file_info where sha256=#{sha256}")
    public List<String> select(byte[] sha256);
}
