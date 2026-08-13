package xyz.suonan.myfolder_sever.Dao;

import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Select;
import xyz.suonan.myfolder_sever.pojo.User;

public interface UserDao {
    @Insert("insert into user values (#{account},#{password})")
    void insert(User user);
    @Select("select * from user where account=#{account}")
    User findOne(User user);
}
