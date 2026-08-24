package xyz.suonan.myfolder_sever.device.api;

import java.util.List;

public record DeviceHeartbeatRequest(String deviceAddress, Integer listenPort, String clientVersion,
                                     List<String> localAddresses) {
    public DeviceHeartbeatRequest(String deviceAddress, Integer listenPort, String clientVersion) {
        this(deviceAddress, listenPort, clientVersion, List.of());
    }
}
