package xyz.suonan.myfolder_sever.forward.service;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.annotation.PostConstruct;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.web.socket.TextMessage;
import xyz.suonan.myfolder_sever.MyObject.Device.Device;
import xyz.suonan.myfolder_sever.Utils.SafeRelativePath;
import xyz.suonan.myfolder_sever.device.service.DeviceService;
import xyz.suonan.myfolder_sever.forward.api.CreateForwardTaskRequest;
import xyz.suonan.myfolder_sever.forward.model.*;
import xyz.suonan.myfolder_sever.transfer.error.TransferException;
import xyz.suonan.myfolder_sever.transfer.model.UploadFile;
import xyz.suonan.myfolder_sever.transfer.model.UploadTask;
import xyz.suonan.myfolder_sever.transfer.service.ResumableTransferService;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.time.Instant;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@Service
public class ForwardTaskService {
    private static final Pattern RANGE = Pattern.compile("bytes=(\\d*)-(\\d*)");

    private final DeviceService deviceService;
    private final ResumableTransferService transferService;
    private final ObjectMapper objectMapper;
    private final Map<String, ForwardTask> tasks = new ConcurrentHashMap<>();
    private final Map<String, Object> taskLocks = new ConcurrentHashMap<>();
    private final Path metadataFile;

    public ForwardTaskService(DeviceService deviceService, ResumableTransferService transferService,
                              ObjectMapper objectMapper,
                              @Value("${forward.metadata-file:${transfer.storage-root}/.myfolder/forwards.json}") String metadataFile) {
        this.deviceService = deviceService;
        this.transferService = transferService;
        this.objectMapper = objectMapper;
        this.metadataFile = Path.of(metadataFile).toAbsolutePath().normalize();
    }

    @PostConstruct
    public void load() {
        if (!Files.exists(metadataFile)) return;
        try {
            List<ForwardTask> saved = objectMapper.readValue(metadataFile.toFile(), new TypeReference<>() {});
            saved.forEach(task -> tasks.put(task.getForwardId(), task));
        } catch (IOException e) {
            throw new IllegalStateException("Cannot load forward task metadata", e);
        }
    }

    public ForwardTask create(String userId, String callerDeviceId, String deviceToken,
                              CreateForwardTaskRequest request) {
        Device caller = authenticate(userId, callerDeviceId, deviceToken);
        if (request == null || blank(request.sourceDeviceId()) || blank(request.targetDeviceId())
                || request.sourceDeviceId().equals(request.targetDeviceId())
                || request.files() == null || request.files().isEmpty()) {
            throw invalid("Source device, target device and files are required");
        }
        if (!caller.getDeviceId().equals(request.sourceDeviceId())) throw notFound();
        Device source = deviceService.find(userId, request.sourceDeviceId());
        Device target = deviceService.find(userId, request.targetDeviceId());
        if ("WEB".equalsIgnoreCase(target.getDeviceType())) {
            throw invalid("WEB devices cannot receive forwarded files");
        }
        String destinationPath = safePath(request.destinationPath(), "destinationPath");
        List<ForwardFile> files = validateFiles(request.files());
        ForwardChannel channel = resolveChannel(request.channel(), source, target);
        validateRelay(userId, channel, request.relayUploadId(), files);

        ForwardTask task = new ForwardTask();
        task.setForwardId(UUID.randomUUID().toString());
        task.setUserId(userId);
        task.setSourceDeviceId(source.getDeviceId());
        task.setTargetDeviceId(target.getDeviceId());
        task.setDestinationPath(destinationPath);
        task.setDeleteSource(request.deleteSource());
        task.setChannel(channel);
        task.setRelayUploadId(request.relayUploadId());
        task.setFiles(files);
        task.setTotalBytes(sumBytes(files));
        task.setState(ForwardState.OFFERED);
        task.setCreatedAt(Instant.now());
        task.setUpdatedAt(task.getCreatedAt());
        tasks.put(task.getForwardId(), task);
        try {
            save();
        } catch (RuntimeException exception) {
            tasks.remove(task.getForwardId());
            throw exception;
        }
        notify(target, "task.forward.offer", task);
        return task;
    }

    public List<ForwardTask> list(String userId, String callerDeviceId, String deviceToken) {
        Device caller = authenticate(userId, callerDeviceId, deviceToken);
        return tasks.values().stream()
                .filter(task -> userId.equals(task.getUserId()) && participant(task, caller.getDeviceId()))
                .sorted(Comparator.comparing(ForwardTask::getCreatedAt).reversed())
                .toList();
    }

    public ForwardTask get(String userId, String callerDeviceId, String deviceToken, String forwardId) {
        Device caller = authenticate(userId, callerDeviceId, deviceToken);
        ForwardTask task = owned(userId, forwardId);
        requireParticipant(task, caller.getDeviceId());
        return task;
    }

    public ForwardTask accept(String userId, String deviceId, String deviceToken, String forwardId) {
        return mutateAsTarget(userId, deviceId, deviceToken, forwardId, task -> {
            require(task, ForwardState.OFFERED);
            task.setState(ForwardState.ACCEPTED);
            task.setFailureReason(null);
            notifyAfterSave(task, "task.forward.accepted", task.getSourceDeviceId());
        });
    }

    public ForwardTask start(String userId, String deviceId, String deviceToken, String forwardId) {
        return mutateAsTarget(userId, deviceId, deviceToken, forwardId, task -> {
            if (task.getState() != ForwardState.ACCEPTED && task.getState() != ForwardState.TRANSFERRING
                    && task.getState() != ForwardState.FAILED) {
                throw conflict("Forward task must be accepted before transfer starts");
            }
            task.setState(ForwardState.TRANSFERRING);
            task.setFailureReason(null);
        });
    }

    public ForwardTask progress(String userId, String deviceId, String deviceToken,
                                String forwardId, long transferredBytes) {
        return mutateAsTarget(userId, deviceId, deviceToken, forwardId, task -> {
            require(task, ForwardState.TRANSFERRING);
            if (transferredBytes < task.getTransferredBytes() || transferredBytes > task.getTotalBytes()) {
                throw invalid("transferredBytes must be monotonic and within task bounds");
            }
            task.setTransferredBytes(transferredBytes);
        });
    }

    public ForwardTask complete(String userId, String deviceId, String deviceToken, String forwardId) {
        return mutateAsTarget(userId, deviceId, deviceToken, forwardId, task -> {
            require(task, ForwardState.TRANSFERRING);
            if (task.getTransferredBytes() != task.getTotalBytes()) {
                throw new ForwardException("FORWARD_INCOMPLETE", "Forward task still has bytes to transfer");
            }
            task.setState(ForwardState.COMPLETED);
            notifyAfterSave(task, "task.forward.completed", task.getSourceDeviceId());
        });
    }

    public ForwardTask fail(String userId, String deviceId, String deviceToken,
                            String forwardId, String reason) {
        return mutateAsTarget(userId, deviceId, deviceToken, forwardId, task -> {
            if (task.getState() != ForwardState.ACCEPTED && task.getState() != ForwardState.TRANSFERRING) {
                throw conflict("Forward task cannot fail in its current state");
            }
            task.setState(ForwardState.FAILED);
            task.setFailureReason(blank(reason) ? "Client reported transfer failure" : reason);
        });
    }

    public ForwardTask cancel(String userId, String deviceId, String deviceToken, String forwardId) {
        Device caller = authenticate(userId, deviceId, deviceToken);
        Object lock = taskLocks.computeIfAbsent(forwardId, ignored -> new Object());
        synchronized (lock) {
            ForwardTask task = owned(userId, forwardId);
            requireParticipant(task, caller.getDeviceId());
            requireNotTerminal(task);
            task.setState(ForwardState.CANCELLED);
            touch(task);
            save();
            String otherDevice = caller.getDeviceId().equals(task.getSourceDeviceId())
                    ? task.getTargetDeviceId() : task.getSourceDeviceId();
            notify(deviceService.find(userId, otherDevice), "task.forward.cancelled", task);
            return task;
        }
    }

    public RelayContent relayContent(String userId, String deviceId, String deviceToken,
                                     String forwardId, String requestedPath, String rangeHeader) {
        Device caller = authenticate(userId, deviceId, deviceToken);
        Object lock = taskLocks.computeIfAbsent(forwardId, ignored -> new Object());
        synchronized (lock) {
            ForwardTask task = owned(userId, forwardId);
            requireTarget(task, caller.getDeviceId());
            if (task.getChannel() != ForwardChannel.RELAY || blank(task.getRelayUploadId())) {
                throw relayNotReady("Forward task does not have relay content");
            }
            if (task.getState() != ForwardState.ACCEPTED && task.getState() != ForwardState.TRANSFERRING
                    && task.getState() != ForwardState.FAILED) {
                throw conflict("Forward task is not ready for download");
            }
            validateRelay(userId, ForwardChannel.RELAY, task.getRelayUploadId(), task.getFiles());
            String path = safePath(requestedPath, "path");
            ForwardFile manifest = task.getFiles().stream().filter(file -> file.path().equals(path)).findFirst()
                    .orElseThrow(() -> new ForwardException("FORWARD_FILE_NOT_FOUND", "File is not in the forward manifest"));
            try {
                Path contentPath = transferService.completedFilePath(userId, task.getRelayUploadId(), path);
                long size = Files.size(contentPath);
                if (size != manifest.size()) throw relayNotReady("Relay file size no longer matches the manifest");
                long[] range = parseRange(rangeHeader, size);
                return new RelayContent(contentPath, manifest.path(), manifest.sha256(), size,
                        range[0], range[1], rangeHeader != null && !rangeHeader.isBlank());
            } catch (TransferException | IOException exception) {
                throw relayNotReady("Relay file is not available");
            }
        }
    }

    private ForwardTask mutateAsTarget(String userId, String deviceId, String deviceToken,
                                       String forwardId, TaskMutation mutation) {
        Device caller = authenticate(userId, deviceId, deviceToken);
        Object lock = taskLocks.computeIfAbsent(forwardId, ignored -> new Object());
        synchronized (lock) {
            ForwardTask task = owned(userId, forwardId);
            requireTarget(task, caller.getDeviceId());
            requireNotTerminal(task);
            mutation.apply(task);
            touch(task);
            try {
                save();
            } catch (RuntimeException exception) {
                notifications.remove(forwardId);
                throw exception;
            }
            dispatchDeferredNotification(task);
            return task;
        }
    }

    private final Map<String, PendingNotification> notifications = new ConcurrentHashMap<>();

    private void notifyAfterSave(ForwardTask task, String action, String deviceId) {
        notifications.put(task.getForwardId(), new PendingNotification(action, deviceId));
    }

    private void dispatchDeferredNotification(ForwardTask task) {
        PendingNotification notification = notifications.remove(task.getForwardId());
        if (notification != null) {
            notify(deviceService.find(task.getUserId(), notification.deviceId()), notification.action(), task);
        }
    }

    private Device authenticate(String userId, String deviceId, String deviceToken) {
        try {
            return deviceService.authenticate(userId, deviceId, deviceToken);
        } catch (DeviceService.DeviceException exception) {
            throw notFound();
        }
    }

    private ForwardTask owned(String userId, String id) {
        ForwardTask task = tasks.get(id);
        if (task == null || !userId.equals(task.getUserId())) throw notFound();
        return task;
    }

    private void requireParticipant(ForwardTask task, String deviceId) {
        if (!participant(task, deviceId)) throw notFound();
    }

    private boolean participant(ForwardTask task, String deviceId) {
        return deviceId.equals(task.getSourceDeviceId()) || deviceId.equals(task.getTargetDeviceId());
    }

    private void requireTarget(ForwardTask task, String deviceId) {
        if (!deviceId.equals(task.getTargetDeviceId())) throw notFound();
    }

    private void require(ForwardTask task, ForwardState state) {
        if (task.getState() != state) throw conflict("Forward task is not in " + state + " state");
    }

    private void requireNotTerminal(ForwardTask task) {
        if (task.getState() == ForwardState.COMPLETED || task.getState() == ForwardState.CANCELLED) {
            throw conflict("Forward task is terminal");
        }
    }

    private List<ForwardFile> validateFiles(List<ForwardFile> requestedFiles) {
        Set<String> paths = new HashSet<>();
        List<ForwardFile> result = new ArrayList<>();
        for (ForwardFile file : requestedFiles) {
            if (file == null || file.size() < 0 || blank(file.sha256())
                    || !file.sha256().matches("[0-9a-fA-F]{64}")) {
                throw invalid("Each file needs a safe path, non-negative size and SHA-256");
            }
            String path = safePath(file.path(), "file path");
            if (!paths.add(path)) throw invalid("File paths must be unique");
            result.add(new ForwardFile(path, file.size(), file.sha256().toLowerCase(Locale.ROOT)));
        }
        sumBytes(result);
        return List.copyOf(result);
    }

    private String safePath(String value, String field) {
        try {
            return SafeRelativePath.normalize(value);
        } catch (IllegalArgumentException exception) {
            throw new ForwardException("INVALID_PATH", field + " must be a safe relative path");
        }
    }

    private long sumBytes(List<ForwardFile> files) {
        long total = 0;
        try {
            for (ForwardFile file : files) total = Math.addExact(total, file.size());
        } catch (ArithmeticException exception) {
            throw invalid("Forward totalBytes overflow");
        }
        return total;
    }

    private ForwardChannel resolveChannel(ForwardChannel requested, Device source, Device target) {
        if (requested != null && requested != ForwardChannel.AUTO) return requested;
        if (source.getListenPort() != null && target.getListenPort() != null
                && sameSubnet(source.getDeviceAddress(), target.getDeviceAddress())) return ForwardChannel.LAN;
        return ForwardChannel.RELAY;
    }

    private void validateRelay(String userId, ForwardChannel channel, String relayUploadId,
                               List<ForwardFile> files) {
        if (channel != ForwardChannel.RELAY) {
            if (!blank(relayUploadId)) throw invalid("relayUploadId must be null for LAN and P2P");
            return;
        }
        if (blank(relayUploadId)) throw relayNotReady("relayUploadId is required for RELAY");
        try {
            UploadTask upload = transferService.completedForRelay(userId, relayUploadId);
            if (upload.totalFiles != files.size() || upload.totalBytes != sumBytes(files)
                    || upload.files.size() != files.size()) {
                throw relayNotReady("Relay upload manifest does not match the forward manifest");
            }
            Map<String, ForwardFile> expected = new HashMap<>();
            files.forEach(file -> expected.put(file.path(), file));
            for (UploadFile uploaded : upload.files.values()) {
                ForwardFile file = expected.get(uploaded.path);
                if (file == null || file.size() != uploaded.size || !file.sha256().equalsIgnoreCase(uploaded.sha256)) {
                    throw relayNotReady("Relay upload manifest does not match the forward manifest");
                }
            }
        } catch (TransferException exception) {
            throw relayNotReady("Relay upload is missing, unauthorized or incomplete");
        }
    }

    private long[] parseRange(String value, long size) {
        if (value == null || value.isBlank()) return new long[]{0, size == 0 ? -1 : size - 1};
        if (size == 0) throw rangeError(size);
        Matcher matcher = RANGE.matcher(value.trim());
        if (!matcher.matches() || (matcher.group(1).isEmpty() && matcher.group(2).isEmpty())) {
            throw rangeError(size);
        }
        try {
            long start;
            long end;
            if (matcher.group(1).isEmpty()) {
                long suffix = Long.parseLong(matcher.group(2));
                if (suffix <= 0) throw rangeError(size);
                start = Math.max(0, size - suffix);
                end = size - 1;
            } else {
                start = Long.parseLong(matcher.group(1));
                end = matcher.group(2).isEmpty() ? size - 1 : Long.parseLong(matcher.group(2));
                if (start < 0 || start >= size || end < start || end >= size) throw rangeError(size);
            }
            return new long[]{start, end};
        } catch (NumberFormatException exception) {
            throw rangeError(size);
        }
    }

    private boolean sameSubnet(String left, String right) {
        if (blank(left) || blank(right)) return false;
        String[] a = left.split("\\.");
        String[] b = right.split("\\.");
        return a.length == 4 && b.length == 4 && a[0].equals(b[0]) && a[1].equals(b[1]) && a[2].equals(b[2]);
    }

    private void notify(Device device, String action, ForwardTask task) {
        if (device == null || device.getWebSocketSession() == null || !device.getWebSocketSession().isOpen()) return;
        try {
            Map<String, Object> message = new LinkedHashMap<>();
            message.put("action", action);
            message.put("timestamp", Instant.now().toString());
            message.put("msgId", task.getForwardId());
            message.put("payload", task);
            String serialized = objectMapper.writeValueAsString(message);
            synchronized (device.getWebSocketSession()) {
                if (device.getWebSocketSession().isOpen()) {
                    device.getWebSocketSession().sendMessage(new TextMessage(serialized));
                }
            }
        } catch (Exception ignored) {
        }
    }

    private synchronized void save() {
        try {
            Files.createDirectories(metadataFile.getParent());
            Path temporary = metadataFile.resolveSibling(metadataFile.getFileName() + ".tmp");
            objectMapper.writeValue(temporary.toFile(), tasks.values());
            try {
                Files.move(temporary, metadataFile, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
            } catch (java.nio.file.AtomicMoveNotSupportedException ignored) {
                Files.move(temporary, metadataFile, StandardCopyOption.REPLACE_EXISTING);
            }
        } catch (IOException e) {
            throw new IllegalStateException("Cannot persist forward task metadata", e);
        }
    }

    private void touch(ForwardTask task) {
        task.setUpdatedAt(Instant.now());
    }

    private boolean blank(String value) {
        return value == null || value.isBlank();
    }

    private ForwardException invalid(String message) {
        return new ForwardException("INVALID_REQUEST", message);
    }

    private ForwardException conflict(String message) {
        return new ForwardException("TASK_STATE_CONFLICT", message);
    }

    private ForwardException relayNotReady(String message) {
        return new ForwardException("RELAY_NOT_READY", message);
    }

    private ForwardException rangeError(long size) {
        return new ForwardException("RANGE_NOT_SATISFIABLE", "Requested range is outside the file",
                Map.of("fileSize", size));
    }

    private ForwardException notFound() {
        return new ForwardException("FORWARD_NOT_FOUND", "Forward task was not found");
    }

    private interface TaskMutation {
        void apply(ForwardTask task);
    }

    private record PendingNotification(String action, String deviceId) {
    }

    public record RelayContent(Path path, String filePath, String sha256, long fileSize,
                               long start, long end, boolean partial) {
        public long contentLength() {
            return end < start ? 0 : end - start + 1;
        }
    }

    public static class ForwardException extends RuntimeException {
        private final String code;
        private final Map<String, Object> details;

        public ForwardException(String code, String message) {
            this(code, message, Map.of());
        }

        public ForwardException(String code, String message, Map<String, Object> details) {
            super(message);
            this.code = code;
            this.details = details == null ? Map.of() : Map.copyOf(details);
        }

        public String getCode() {
            return code;
        }

        public Map<String, Object> getDetails() {
            return details;
        }
    }
}
