package xyz.suonan.myfolder_sever.Service.Session;

import org.springframework.stereotype.Component;
import xyz.suonan.myfolder_sever.MyObject.Device;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@Component
public class SessionRepository {
    private final Map<String, Map<String,Device>> sessions = new ConcurrentHashMap<>();

    public void register(String userId,Device device) {
        if(sessions.get(userId)==null){
            Map<String,Device> deviceMap=new ConcurrentHashMap<>();
            deviceMap.put(device.getDeviceId(),device);
            sessions.put(userId,deviceMap);
        }else{
            sessions.get(userId).put(device.getDeviceId(),device);
        }
    }

    public void unregister(String userId,Device device) {
        sessions.get(userId).remove(device.getDeviceId());


    }
}
