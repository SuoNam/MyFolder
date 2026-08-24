package xyz.suonan.myfolder_sever.device.api;

import java.util.List;

public record DeviceRegistrationRequest(
        String deviceId,
        String deviceName,
        String deviceType,
        String os,
        String clientVersion,
        String deviceAddress,
        Integer listenPort,
        List<String> localAddresses) {

    public DeviceRegistrationRequest(String deviceId, String deviceName, String deviceType,
                                     String os, String clientVersion, String deviceAddress,
                                     Integer listenPort) {
        this(deviceId, deviceName, deviceType, os, clientVersion, deviceAddress, listenPort, List.of());
    }
}
