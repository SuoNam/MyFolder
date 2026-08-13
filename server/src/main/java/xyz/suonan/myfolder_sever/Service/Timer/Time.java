package xyz.suonan.myfolder_sever.Service.Timer;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;
import java.util.Map;
@Data
@AllArgsConstructor
public class Time {
    List<Map<String,Long>> partTimeList ;
    Long totalDuration;
}
