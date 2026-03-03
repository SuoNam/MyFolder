package xyz.suonan.myfolder_sever.MyObject.Device;

import lombok.AllArgsConstructor;
import lombok.Getter;
import org.springframework.web.socket.WebSocketSession;
@AllArgsConstructor
@Getter
public class Device {
    String deviceId;
    String deviceName;
    String deviceType;
    String deviceAddress;
    WebSocketSession webSocketSession;
}
