package xyz.suonan.myfolder_sever.MyObject.Device;

import lombok.Getter;
import lombok.Setter;
import java.time.Instant;

@Getter
@Setter
public class DeviceInfo {
    private String deviceId;
    private String userId;
    private String deviceName;
    private String deviceType;
    private String os;
    private String deviceAddress;
    private Integer listenPort;
    private String clientVersion;
    private Instant createdAt;
    private Instant lastSeenAt;
    private boolean online;
}
