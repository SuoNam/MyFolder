package xyz.suonan.myfolder_sever.Service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import xyz.suonan.myfolder_sever.Dao.UserDao;
import xyz.suonan.myfolder_sever.pojo.User;

@Service
public class UserService {
    private static final Logger log = LoggerFactory.getLogger(UserService.class);
    @Autowired
    UserDao userDao;
    @Autowired
    private PasswordEncoder passwordEncoder;
    public boolean addUser(User user) {
        try{
            user.password=passwordEncoder.encode(user.getPassword());
            userDao.insert(user);
        }catch(Exception e){
            log.warn("注册用户失败: account={}", user == null ? null : user.account, e);
            return false;
        }
        return true;
    }
    public boolean judgeUser(User user) {
        if (user == null || user.account == null || user.account.isBlank()
                || user.password == null || user.password.isBlank()) return false;
        User goaluser=userDao.findOne(user);
        if (goaluser == null || goaluser.password == null || goaluser.password.isBlank()) return false;
        return passwordEncoder.matches(user.password, goaluser.password);
    }

    /** @deprecated use {@link #judgeUser(User)} */
    @Deprecated
    public boolean JudgeUser(User user) { return judgeUser(user); }
}
