package xyz.suonan.myfolder_sever.device.service;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.annotation.PostConstruct;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.MyObject.Device.Device;
import xyz.suonan.myfolder_sever.MyObject.Device.DeviceInfo;
import xyz.suonan.myfolder_sever.device.api.DeviceHeartbeatRequest;
import xyz.suonan.myfolder_sever.device.api.DeviceRegistrationRequest;

import java.security.SecureRandom;
import java.security.MessageDigest;
import java.nio.charset.StandardCharsets;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.time.Instant;
import java.util.Base64;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

@Service
public class DeviceService {
    private final Map<String, Device> devices = new ConcurrentHashMap<>();
    private final SecureRandom secureRandom = new SecureRandom();
    private final ObjectMapper objectMapper;
    private final Path metadataFile;

    public DeviceService(ObjectMapper objectMapper,
                         @Value("${device.metadata-file:${transfer.storage-root}/.myfolder/devices.json}") String metadataFile) {
        this.objectMapper = objectMapper;
        this.metadataFile = Path.of(metadataFile).toAbsolutePath().normalize();
    }

    @PostConstruct
    public void load() {
        if (!Files.exists(metadataFile)) return;
        try {
            List<Device> saved = objectMapper.readValue(metadataFile.toFile(), new TypeReference<>() {});
            Instant now = Instant.now();
            for (Device device : saved) {
                device.setOnline(false);
                device.setWebSocketSession(null);
                if (device.getLastSeenAt() == null) device.setLastSeenAt(now);
                devices.put(key(device.getUserId(), device.getDeviceId()), device);
            }
        } catch (IOException e) {
            throw new IllegalStateException("Cannot load device metadata", e);
        }
    }

    public DeviceRegistrationResult register(String userId, DeviceRegistrationRequest request) {
        if (userId == null || userId.isBlank() || request == null || blank(request.deviceName())
                || blank(request.deviceType()) || blank(request.os()) || blank(request.clientVersion())) {
            throw new DeviceException("INVALID_REQUEST", "deviceName, deviceType, os and clientVersion are required");
        }
        String deviceId = blank(request.deviceId()) ? UUID.randomUUID().toString() : request.deviceId().trim();
        String key = key(userId, deviceId);
        Instant now = Instant.now();
        Device device = devices.compute(key, (ignored, old) -> {
            if (old == null) {
                return new Device(deviceId, userId, request.deviceName().trim(), request.deviceType().trim(),
                        request.os().trim(), request.deviceAddress(), validPort(request.listenPort()), request.clientVersion().trim(),
                        newToken(), now, now, true, null);
            }
            old.setDeviceName(request.deviceName().trim());
            old.setDeviceType(request.deviceType().trim());
            old.setOs(request.os().trim());
            old.setDeviceAddress(request.deviceAddress());
            old.setListenPort(validPort(request.listenPort()));
            old.setClientVersion(request.clientVersion().trim());
            if (blank(old.getDeviceToken())) old.setDeviceToken(newToken());
            old.setLastSeenAt(now);
            old.setOnline(true);
            return old;
        });
        save();
        return new DeviceRegistrationResult(toInfo(device), device.getDeviceToken());
    }

    public List<DeviceInfo> list(String userId) {
        return devices.values().stream()
                .filter(device -> userId.equals(device.getUserId()))
                .sorted(Comparator.comparing(Device::getDeviceName, String.CASE_INSENSITIVE_ORDER))
                .map(this::toInfo)
                .toList();
    }

    public DeviceInfo get(String userId, String deviceId) {
        Device device = find(userId, deviceId);
        return toInfo(device);
    }

    public DeviceInfo heartbeat(String userId, String deviceId, DeviceHeartbeatRequest request) {
        Device device = find(userId, deviceId);
        Instant now = Instant.now();
        device.setOnline(true);
        device.setLastSeenAt(now);
        if (request != null && !blank(request.deviceAddress())) device.setDeviceAddress(request.deviceAddress());
        if (request != null && request.listenPort() != null) device.setListenPort(validPort(request.listenPort()));
        if (request != null && !blank(request.clientVersion())) device.setClientVersion(request.clientVersion());
        save();
        return toInfo(device);
    }

    public DeviceInfo rename(String userId, String deviceId, String deviceToken, String deviceName) {
        Device device = authenticate(userId, deviceId, deviceToken);
        if (blank(deviceName) || deviceName.trim().length() > 64) {
            throw new DeviceException("INVALID_REQUEST", "deviceName must contain 1 to 64 characters");
        }
        device.setDeviceName(deviceName.trim());
        device.setLastSeenAt(Instant.now());
        save();
        return toInfo(device);
    }

    public void unregister(String userId, String deviceId) {
        Device device = find(userId, deviceId);
        device.setOnline(false);
        device.setLastSeenAt(Instant.now());
        save();
    }

    public Device find(String userId, String deviceId) {
        Device device = devices.get(key(userId, deviceId));
        if (device == null) throw new DeviceException("DEVICE_NOT_FOUND", "Device was not found");
        return device;
    }

    public Device authenticate(String userId, String deviceId, String deviceToken) {
        Device device = find(userId, deviceId);
        if (deviceToken == null || device.getDeviceToken() == null || !MessageDigest.isEqual(
                device.getDeviceToken().getBytes(StandardCharsets.UTF_8),
                deviceToken.getBytes(StandardCharsets.UTF_8))) {
            throw new DeviceException("DEVICE_NOT_FOUND", "Device was not found");
        }
        return device;
    }

    public Device findByToken(String deviceToken) {
        return devices.values().stream().filter(device -> device.getDeviceToken().equals(deviceToken)).findFirst()
                .orElseThrow(() -> new DeviceException("DEVICE_NOT_FOUND", "Device was not found"));
    }

    public void attachSession(String userId, String deviceId, org.springframework.web.socket.WebSocketSession session) {
        Device device = find(userId, deviceId);
        device.setWebSocketSession(session);
        device.setOnline(true);
        device.setLastSeenAt(Instant.now());
        save();
    }

    public void detachSession(String userId, String deviceId, org.springframework.web.socket.WebSocketSession session) {
        Device device = devices.get(key(userId, deviceId));
        if (device != null && device.getWebSocketSession() == session) {
            device.setWebSocketSession(null);
            device.setOnline(false);
            device.setLastSeenAt(Instant.now());
            save();
        }
    }

    private DeviceInfo toInfo(Device device) {
        DeviceInfo info = new DeviceInfo();
        info.setDeviceId(device.getDeviceId());
        info.setUserId(device.getUserId());
        info.setDeviceName(device.getDeviceName());
        info.setDeviceType(device.getDeviceType());
        info.setOs(device.getOs());
        info.setDeviceAddress(device.getDeviceAddress());
        info.setListenPort(device.getListenPort());
        info.setClientVersion(device.getClientVersion());
        info.setCreatedAt(device.getCreatedAt());
        info.setLastSeenAt(device.getLastSeenAt());
        info.setOnline(device.isOnline());
        return info;
    }

    private String key(String userId, String deviceId) { return userId + "\u0000" + deviceId; }
    private boolean blank(String value) { return value == null || value.isBlank(); }
    private Integer validPort(Integer port) {
        if (port != null && (port < 1 || port > 65535)) {
            throw new DeviceException("INVALID_REQUEST", "listenPort must be between 1 and 65535");
        }
        return port;
    }
    private String newToken() {
        byte[] value = new byte[32];
        secureRandom.nextBytes(value);
        return Base64.getUrlEncoder().withoutPadding().encodeToString(value);
    }

    private synchronized void save() {
        try {
            Files.createDirectories(metadataFile.getParent());
            Path temporary = metadataFile.resolveSibling(metadataFile.getFileName() + ".tmp");
            objectMapper.writeValue(temporary.toFile(), devices.values());
            try {
                Files.move(temporary, metadataFile, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
            } catch (java.nio.file.AtomicMoveNotSupportedException ignored) {
                Files.move(temporary, metadataFile, StandardCopyOption.REPLACE_EXISTING);
            }
        } catch (IOException e) {
            throw new IllegalStateException("Cannot persist device metadata", e);
        }
    }

    public record DeviceRegistrationResult(DeviceInfo device, String deviceToken) {}

    public static class DeviceException extends RuntimeException {
        private final String code;
        public DeviceException(String code, String message) { super(message); this.code = code; }
        public String getCode() { return code; }
    }
}
