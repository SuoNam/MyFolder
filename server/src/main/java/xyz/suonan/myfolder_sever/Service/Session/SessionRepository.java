package xyz.suonan.myfolder_sever.Service.Session;

import org.springframework.stereotype.Component;
import xyz.suonan.myfolder_sever.MyObject.Device.Device;
import xyz.suonan.myfolder_sever.MyObject.Device.DeviceInfo;

import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

@Component
public class SessionRepository {
    private final Map<String, Map<String,Device>> sessions = new ConcurrentHashMap<>();

    public void register(String userId,Device device) {
        sessions.computeIfAbsent(userId, k -> new ConcurrentHashMap<>())
                .put(device.getDeviceId(), device);
    }

    public void unregister(String userId,String deviceId) {
        Map<String, Device> deviceMap = sessions.get(userId);
        if (deviceMap != null) {
            deviceMap.remove(deviceId);
            if (deviceMap.isEmpty()) {
                sessions.remove(userId);
            }
        }
    }

    public List<DeviceInfo> getDevices(String userId) {
        Map<String, Device> devicesMap = sessions.get(userId);
        if (devicesMap == null || devicesMap.isEmpty()) {
            return List.of();
        }

        return devicesMap.values().stream().map(device -> {
            DeviceInfo info = new DeviceInfo();
            info.setDeviceId(device.getDeviceId());
            info.setUserId(device.getUserId());
            info.setDeviceName(device.getDeviceName());
            info.setDeviceType(device.getDeviceType());
            info.setOs(device.getOs());
            info.setDeviceAddress(device.getDeviceAddress());
            info.setClientVersion(device.getClientVersion());
            info.setCreatedAt(device.getCreatedAt());
            info.setLastSeenAt(device.getLastSeenAt());
            info.setOnline(device.isOnline());
            return info;
        }).collect(Collectors.toList());
    }

    public Device getSpecificDevice(String userId, String deviceId) {
        Map<String, Device> devices = sessions.get(userId);
        return (devices != null) ? devices.get(deviceId) : null;
    }
    }
