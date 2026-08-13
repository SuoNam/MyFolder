package xyz.suonan.myfolder_sever.MyObject.Device;

import com.fasterxml.jackson.annotation.JsonIgnore;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import org.springframework.web.socket.WebSocketSession;
import java.time.Instant;
@AllArgsConstructor
@NoArgsConstructor
@Getter
@Setter
public class Device {
    private String deviceId;
    private String userId;
    private String deviceName;
    private String deviceType;
    private String os;
    private String deviceAddress;
    private Integer listenPort;
    private String clientVersion;
    private String deviceToken;
    private Instant createdAt;
    private Instant lastSeenAt;
    private boolean online;
    @JsonIgnore
    private transient WebSocketSession webSocketSession;
}
