package xyz.suonan.myfolder_sever.Controller;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Service.UserService;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import xyz.suonan.myfolder_sever.pojo.User;

import java.util.HashMap;
import java.util.Map;

@RequestMapping("/user")
@RestController
public class UserController {
    @Autowired
    UserService userService;

    @Autowired
    JwtGen jwtGen;

    @PostMapping("/signup")
    public BaseMessage<Map<String,String>> signUp(@RequestBody User user) {
        if(!userService.addUser(user)){
            return new BaseMessage<>(500,"注册失败",null);
        }
        //TODO::添加注册成功返回Token
        Map<String,String> tokenMap = new HashMap<>();
        tokenMap.put("token",jwtGen.genJwt(user.account));
        return new BaseMessage<>(200,"注册成功",tokenMap);
    }
    @PostMapping("/login")
    public BaseMessage<Map<String,String>> logIn(@RequestBody User user) {
        if(!userService.JudgeUser(user)){
            return new BaseMessage<>(500,"登录失败",null);
        }
        //TODO::添加登录成功返回Token
        Map<String,String> tokenMap = new HashMap<>();
        tokenMap.put("token",jwtGen.genJwt(user.account));
        return new BaseMessage<>(200,"登录成功",tokenMap);


    }
}
