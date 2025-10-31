package xyz.suonan.myfolder_sever.Controller;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Service.Timer.Time;
import xyz.suonan.myfolder_sever.Service.Timer.TotalTimer;

import java.util.Queue;

@RestController("/time")
public class TimeController {

    @Autowired
    TotalTimer  totalTimer;

    @GetMapping("/directory")
    public BaseMessage<Queue<Time>> directory() {
        return new BaseMessage<>(200,"success",totalTimer.getTimeQueue());
    }

}
