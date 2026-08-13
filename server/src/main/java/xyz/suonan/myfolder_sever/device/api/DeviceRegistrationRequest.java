package xyz.suonan.myfolder_sever.device.api;

public record DeviceRegistrationRequest(
        String deviceId,
        String deviceName,
        String deviceType,
        String os,
        String clientVersion,
        String deviceAddress,
        Integer listenPort) {
}
