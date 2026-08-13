package xyz.suonan.myfolder_sever.device.api;

public record DeviceHeartbeatRequest(String deviceAddress, Integer listenPort, String clientVersion) {
}
