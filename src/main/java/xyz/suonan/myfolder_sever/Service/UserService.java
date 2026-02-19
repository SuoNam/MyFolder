package xyz.suonan.myfolder_sever.Service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.Dao.UserDao;
import xyz.suonan.myfolder_sever.pojo.User;

@Service
public class UserService {
    @Autowired
    UserDao userDao;
    public boolean addUser(User user) {
        try{
            userDao.insert(user);
        }catch(Exception e){
            System.out.println(e.getMessage());
            return false;
        }
        return true;
    }
    public boolean JudgeUser(User user) {
        User goaluser=userDao.findOne(user);
        return goaluser != null;
    }
}
