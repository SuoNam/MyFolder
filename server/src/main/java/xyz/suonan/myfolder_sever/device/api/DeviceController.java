package xyz.suonan.myfolder_sever.device.api;

import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import lombok.RequiredArgsConstructor;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;
import xyz.suonan.myfolder_sever.MyObject.Device.DeviceInfo;
import xyz.suonan.myfolder_sever.device.service.DeviceService;
import xyz.suonan.myfolder_sever.Config.OpenApiConfig;

import java.util.List;

@RestController
@RequestMapping("/api/v1/devices")
@RequiredArgsConstructor
@Tag(name = "Devices", description = "注册设备、查询在线状态和心跳。")
@SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
public class DeviceController {
    private final DeviceService service;

    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public DeviceRegistrationResponse register(@RequestBody DeviceRegistrationRequest request, HttpServletRequest servletRequest) {
        DeviceService.DeviceRegistrationResult result = service.register(userId(servletRequest), request);
        return new DeviceRegistrationResponse(result.device(), result.deviceToken());
    }

    @GetMapping
    public List<DeviceInfo> list(HttpServletRequest request) { return service.list(userId(request)); }

    @GetMapping("/{deviceId}")
    public DeviceInfo get(@PathVariable String deviceId, HttpServletRequest request) {
        return service.get(userId(request), deviceId);
    }

    @PostMapping("/{deviceId}/heartbeat")
    public DeviceInfo heartbeat(@PathVariable String deviceId, @RequestBody(required = false) DeviceHeartbeatRequest body,
                                HttpServletRequest request) {
        return service.heartbeat(userId(request), deviceId, body);
    }

    @PatchMapping("/{deviceId}")
    public DeviceInfo update(@PathVariable String deviceId, @RequestBody DeviceUpdateRequest body,
                             @RequestHeader("X-Device-Token") String deviceToken,
                             HttpServletRequest request) {
        return service.rename(userId(request), deviceId, deviceToken, body == null ? null : body.deviceName());
    }

    @DeleteMapping("/{deviceId}")
    public DeviceInfo unregister(@PathVariable String deviceId, HttpServletRequest request) {
        service.unregister(userId(request), deviceId);
        return service.get(userId(request), deviceId);
    }

    private String userId(HttpServletRequest request) {
        Object userId = request.getAttribute("myfolder.userId");
        if (userId == null) throw new DeviceService.DeviceException("UNAUTHORIZED", "Authenticated user is required");
        return userId.toString();
    }
}
