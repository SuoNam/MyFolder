package xyz.suonan.myfolder_sever.device.api;

import xyz.suonan.myfolder_sever.MyObject.Device.DeviceInfo;

public record DeviceRegistrationResponse(DeviceInfo device, String deviceToken) {
}
