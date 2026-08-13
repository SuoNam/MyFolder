package xyz.suonan.myfolder_sever.Service.Timer;

import lombok.Data;
import org.springframework.stereotype.Component;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;

@Data
@Component
public class PartTimer implements Timer {
    public String name;
    private long startTime;
    List<Map<String,Long>> partList = new LinkedList<>();

    // 开始计时
    public void begin() {
        startTime = System.currentTimeMillis();
    }

    // 结束计时并打印耗时
    public void close() {
        long endTime = System.currentTimeMillis();
        long duration = endTime - startTime;
        Date date = new Date(endTime);
        DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Map<String, Long> map= new HashMap<>();
        String key= df.format(date) +"|"+name;
        map.put(key,duration);
        partList.add(map);
    }
}
