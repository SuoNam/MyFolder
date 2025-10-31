package xyz.suonan.myfolder_sever.Service.Timer;
import lombok.Data;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
@Data
@Component
public class TotalTimer implements Timer {
    @Autowired
    PartTimer partTimer;
    private long startTime;
    Queue<Time> timeQueue = new LinkedList<>();
    @Override
    public void begin() {
        startTime = System.currentTimeMillis();
    }

    @Override
    public void close() {
        long endTime = System.currentTimeMillis();

        long duration = endTime - startTime;
        List<Map<String,Long>> list=partTimer.getPartList();
        Time time= new Time(list,duration);
        timeQueue.add(time);
    }
}
