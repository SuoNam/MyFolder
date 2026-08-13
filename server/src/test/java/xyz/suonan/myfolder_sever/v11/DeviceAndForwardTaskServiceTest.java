package xyz.suonan.myfolder_sever.v11;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import xyz.suonan.myfolder_sever.device.api.DeviceRegistrationRequest;
import xyz.suonan.myfolder_sever.device.service.DeviceService;
import xyz.suonan.myfolder_sever.forward.api.CreateForwardTaskRequest;
import xyz.suonan.myfolder_sever.forward.model.*;
import xyz.suonan.myfolder_sever.forward.service.ForwardTaskService;
import xyz.suonan.myfolder_sever.transfer.api.CreateUploadTaskRequest;
import xyz.suonan.myfolder_sever.transfer.model.UploadTask;
import xyz.suonan.myfolder_sever.transfer.service.ResumableTransferService;
import xyz.suonan.myfolder_sever.transfer.service.TransferHash;
import xyz.suonan.myfolder_sever.transfer.store.JsonUploadTaskStore;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;

import static org.junit.jupiter.api.Assertions.*;

class DeviceAndForwardTaskServiceTest {
    private static final String USER = "alice";
    private static final String SOURCE = "desktop";
    private static final String TARGET = "laptop";
    private static final String OTHER = "tablet";
    private static final byte[] CONTENT = "hello relay".getBytes(java.nio.charset.StandardCharsets.UTF_8);
    private static final String HASH = TransferHash.sha256(CONTENT);

    @TempDir
    Path temp;

    private ObjectMapper mapper;
    private DeviceService devices;
    private ResumableTransferService transfers;
    private ForwardTaskService forwards;
    private String sourceToken;
    private String targetToken;
    private String otherToken;

    @BeforeEach
    void setUp() {
        mapper = new ObjectMapper().registerModule(new JavaTimeModule());
        devices = new DeviceService(mapper, temp.resolve("devices.json").toString());
        sourceToken = register(USER, SOURCE, "10.0.0.10", 49152);
        targetToken = register(USER, TARGET, "192.168.1.11", 49153);
        otherToken = register(USER, OTHER, "192.168.1.12", 49154);
        transfers = new ResumableTransferService(
                new JsonUploadTaskStore(mapper, temp.resolve("upload-metadata").toString()),
                temp.resolve("storage").toString());
        forwards = new ForwardTaskService(devices, transfers, mapper, temp.resolve("forwards.json").toString());
    }

    @Test
    void persistsDeviceTokenWithoutExposingItInDeviceInfo() {
        DeviceService.DeviceRegistrationResult registration = devices.register(USER,
                new DeviceRegistrationRequest(TARGET, "Laptop", "PC", "Linux", "1.1.1",
                        "192.168.1.11", 49153));
        DeviceService reloaded = new DeviceService(mapper, temp.resolve("devices.json").toString());
        reloaded.load();

        assertEquals(targetToken, registration.deviceToken());
        assertEquals(TARGET, reloaded.authenticate(USER, TARGET, targetToken).getDeviceId());
        assertFalse(mapper.valueToTree(registration.device()).has("deviceToken"));
    }

    @Test
    void renamesOnlyWithTheRegisteredDeviceToken() {
        assertEquals("Study laptop", devices.rename(USER, TARGET, targetToken, "  Study laptop  ").getDeviceName());
        assertEquals("Study laptop", devices.get(USER, TARGET).getDeviceName());
        DeviceService.DeviceException wrongToken = assertThrows(DeviceService.DeviceException.class,
                () -> devices.rename(USER, TARGET, "wrong-token", "Other"));
        assertEquals("DEVICE_NOT_FOUND", wrongToken.getCode());
        DeviceService.DeviceException invalidName = assertThrows(DeviceService.DeviceException.class,
                () -> devices.rename(USER, TARGET, targetToken, " "));
        assertEquals("INVALID_REQUEST", invalidName.getCode());
    }

    @Test
    void relayRequiresOwnedCompletedMatchingUpload() {
        UploadTask completed = completedUpload(USER, "relay-ready");
        assertEquals(ForwardChannel.RELAY, createRelay(completed.uploadId).getChannel());

        assertCode("RELAY_NOT_READY", () -> createRelay(null));
        assertCode("RELAY_NOT_READY", () -> createRelay("AAAAAAAAAAAAAAAAAAAAAAAAAA"));

        UploadTask incomplete = transfers.create(USER, uploadRequest("relay-incomplete", CONTENT, HASH));
        assertCode("RELAY_NOT_READY", () -> createRelay(incomplete.uploadId));

        UploadTask otherOwner = completedUpload("bob", "relay-bob");
        assertCode("RELAY_NOT_READY", () -> createRelay(otherOwner.uploadId));

        CreateForwardTaskRequest mismatch = relayRequest(completed.uploadId,
                List.of(new ForwardFile("docs/data.bin", CONTENT.length + 1L, HASH)));
        assertCode("RELAY_NOT_READY", () -> forwards.create(USER, SOURCE, sourceToken, mismatch));
    }

    @Test
    void enforcesSourceAndTargetRolesWithoutLeakingTask() {
        UploadTask upload = completedUpload(USER, "relay-role");
        assertHidden(() -> forwards.create(USER, OTHER, otherToken, relayRequest(upload.uploadId, manifest())));
        ForwardTask task = createRelay(upload.uploadId);

        assertHidden(() -> forwards.accept(USER, SOURCE, sourceToken, task.getForwardId()));
        assertHidden(() -> forwards.start(USER, SOURCE, sourceToken, task.getForwardId()));
        assertHidden(() -> forwards.progress(USER, SOURCE, sourceToken, task.getForwardId(), 1));
        assertHidden(() -> forwards.complete(USER, SOURCE, sourceToken, task.getForwardId()));
        assertHidden(() -> forwards.get(USER, OTHER, otherToken, task.getForwardId()));
        assertHidden(() -> forwards.relayContent(USER, OTHER, otherToken, task.getForwardId(),
                "docs/data.bin", null));

        assertEquals(task.getForwardId(), forwards.get(USER, TARGET, targetToken, task.getForwardId()).getForwardId());
        assertEquals(ForwardState.ACCEPTED,
                forwards.accept(USER, TARGET, targetToken, task.getForwardId()).getState());
    }

    @Test
    void validatesEveryForwardPathBeforeCreatingTask() {
        List<String> invalid = List.of("../escape", "a/../b", "a/./b", "a//b", "a/", "/tmp/a", "C:/a", "a\\b");
        for (String path : invalid) {
            CreateForwardTaskRequest badDestination = new CreateForwardTaskRequest(SOURCE, TARGET, path,
                    false, ForwardChannel.LAN, null, manifest());
            assertCode("INVALID_PATH", () -> forwards.create(USER, SOURCE, sourceToken, badDestination));
            CreateForwardTaskRequest badFile = new CreateForwardTaskRequest(SOURCE, TARGET, "Downloads",
                    false, ForwardChannel.LAN, null, List.of(new ForwardFile(path, CONTENT.length, HASH)));
            assertCode("INVALID_PATH", () -> forwards.create(USER, SOURCE, sourceToken, badFile));
        }
    }

    @Test
    void rejectsWebDeviceAsForwardTarget() {
        devices.register(USER, new DeviceRegistrationRequest("browser", "Web Console", "WEB", "Win32",
                "1.1.1-web", "10.0.0.20", null));
        CreateForwardTaskRequest request = new CreateForwardTaskRequest(SOURCE, "browser", "Downloads",
                false, ForwardChannel.LAN, null, manifest());

        assertCode("INVALID_REQUEST", () -> forwards.create(USER, SOURCE, sourceToken, request));
    }

    @Test
    void supportsFullAndRepeatableRangeDownloads() throws Exception {
        ForwardTask task = createRelay(completedUpload(USER, "relay-range").uploadId);
        forwards.accept(USER, TARGET, targetToken, task.getForwardId());

        ForwardTaskService.RelayContent full = content(task, null);
        assertFalse(full.partial());
        assertEquals(0, full.start());
        assertEquals(CONTENT.length - 1L, full.end());
        assertArrayEquals(CONTENT, Files.readAllBytes(full.path()));

        ForwardTaskService.RelayContent middle = content(task, "bytes=1-4");
        assertTrue(middle.partial());
        assertEquals(1, middle.start());
        assertEquals(4, middle.end());
        assertEquals(middle.start(), content(task, "bytes=1-4").start());

        assertEquals(CONTENT.length - 3L, content(task, "bytes=-3").start());
        assertEquals(2, content(task, "bytes=" + (CONTENT.length - 2) + "-").contentLength());
        assertCode("RANGE_NOT_SATISFIABLE", () -> content(task, "bytes=" + CONTENT.length + "-"));
        assertCode("RANGE_NOT_SATISFIABLE", () -> content(task, "bytes=0-" + CONTENT.length));
        assertCode("RANGE_NOT_SATISFIABLE", () -> content(task, "not-a-range"));
    }

    @Test
    void taskLocksPreventTerminalStateRollback() throws Exception {
        ForwardTask acceptCancel = createRelay(completedUpload(USER, "relay-accept-cancel").uploadId);
        runTogether(
                () -> forwards.accept(USER, TARGET, targetToken, acceptCancel.getForwardId()),
                () -> forwards.cancel(USER, SOURCE, sourceToken, acceptCancel.getForwardId()));
        assertEquals(ForwardState.CANCELLED,
                forwards.get(USER, SOURCE, sourceToken, acceptCancel.getForwardId()).getState());

        ForwardTask completeCancel = createRelay(completedUpload(USER, "relay-complete-cancel").uploadId);
        forwards.accept(USER, TARGET, targetToken, completeCancel.getForwardId());
        forwards.start(USER, TARGET, targetToken, completeCancel.getForwardId());
        forwards.progress(USER, TARGET, targetToken, completeCancel.getForwardId(), CONTENT.length);
        runTogether(
                () -> forwards.complete(USER, TARGET, targetToken, completeCancel.getForwardId()),
                () -> forwards.cancel(USER, SOURCE, sourceToken, completeCancel.getForwardId()));
        ForwardState state = forwards.get(USER, SOURCE, sourceToken, completeCancel.getForwardId()).getState();
        assertTrue(state == ForwardState.COMPLETED || state == ForwardState.CANCELLED);
    }

    @Test
    void restoresForwardHistoryAfterRestart() {
        ForwardTask task = createRelay(completedUpload(USER, "relay-restart").uploadId);
        ForwardTaskService restarted = new ForwardTaskService(devices, transfers, mapper,
                temp.resolve("forwards.json").toString());
        restarted.load();

        assertEquals(task.getForwardId(), restarted.get(USER, TARGET, targetToken, task.getForwardId()).getForwardId());
        assertEquals(1, restarted.list(USER, SOURCE, sourceToken).size());
        assertTrue(restarted.list(USER, OTHER, otherToken).isEmpty());
    }

    private String register(String user, String id, String address, int port) {
        return devices.register(user, new DeviceRegistrationRequest(id, id, "PC", "Linux", "1.1.1",
                address, port)).deviceToken();
    }

    private UploadTask completedUpload(String owner, String directory) {
        UploadTask task = transfers.create(owner, uploadRequest(directory, CONTENT, HASH));
        transfers.uploadChunk(owner, task.uploadId, "docs/data.bin", 0, 0, CONTENT.length - 1L,
                CONTENT.length, HASH, CONTENT);
        transfers.completeFile(owner, task.uploadId, "docs/data.bin");
        return transfers.completeTask(owner, task.uploadId);
    }

    private CreateUploadTaskRequest uploadRequest(String directory, byte[] content, String hash) {
        return new CreateUploadTaskRequest("relay/" + directory, 65_536, 1, content.length,
                List.of(new CreateUploadTaskRequest.FileManifest("docs/data.bin", content.length, hash, 1)));
    }

    private ForwardTask createRelay(String uploadId) {
        return forwards.create(USER, SOURCE, sourceToken, relayRequest(uploadId, manifest()));
    }

    private CreateForwardTaskRequest relayRequest(String uploadId, List<ForwardFile> files) {
        return new CreateForwardTaskRequest(SOURCE, TARGET, "Downloads", false,
                ForwardChannel.RELAY, uploadId, files);
    }

    private List<ForwardFile> manifest() {
        return List.of(new ForwardFile("docs/data.bin", CONTENT.length, HASH));
    }

    private ForwardTaskService.RelayContent content(ForwardTask task, String range) {
        return forwards.relayContent(USER, TARGET, targetToken, task.getForwardId(), "docs/data.bin", range);
    }

    private void assertCode(String code, org.junit.jupiter.api.function.Executable action) {
        ForwardTaskService.ForwardException exception = assertThrows(ForwardTaskService.ForwardException.class, action);
        assertEquals(code, exception.getCode());
    }

    private void assertHidden(org.junit.jupiter.api.function.Executable action) {
        assertCode("FORWARD_NOT_FOUND", action);
    }

    private void runTogether(Runnable first, Runnable second) throws Exception {
        CountDownLatch ready = new CountDownLatch(2);
        CountDownLatch start = new CountDownLatch(1);
        CompletableFuture<Void> one = CompletableFuture.runAsync(() -> runIgnoringBusinessConflict(first, ready, start));
        CompletableFuture<Void> two = CompletableFuture.runAsync(() -> runIgnoringBusinessConflict(second, ready, start));
        ready.await();
        start.countDown();
        CompletableFuture.allOf(one, two).get();
    }

    private void runIgnoringBusinessConflict(Runnable action, CountDownLatch ready, CountDownLatch start) {
        ready.countDown();
        try {
            start.await();
            action.run();
        } catch (ForwardTaskService.ForwardException ignored) {
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            throw new RuntimeException(exception);
        }
    }
}
