package xyz.suonan.myfolder_sever.Controller;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Service.UserService;
import xyz.suonan.myfolder_sever.pojo.User;

@RequestMapping("/user")
@RestController
public class UserController {
    @Autowired
    UserService userService;

    @PostMapping("/signup")
    public BaseMessage<User> signUp(@RequestBody User user) {
        if(!userService.addUser(user)){
            return new BaseMessage<>(500,"注册失败",null);
        }
        return new BaseMessage<>(200,"注册成功",user);
    }
    @PostMapping("/login")
    public BaseMessage<User> logIn(@RequestBody User user) {
        if(!userService.JudgeUser(user)){
            return new BaseMessage<>(500,"登录失败",null);
        }
        return new BaseMessage<>(200,"登录成功",user);


    }
}
