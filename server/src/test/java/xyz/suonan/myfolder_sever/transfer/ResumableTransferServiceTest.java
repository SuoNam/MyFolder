package xyz.suonan.myfolder_sever.transfer;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import xyz.suonan.myfolder_sever.file.StorageQuotaService;
import xyz.suonan.myfolder_sever.file.StorageScopeService;
import xyz.suonan.myfolder_sever.transfer.api.CreateUploadTaskRequest;
import xyz.suonan.myfolder_sever.transfer.api.UploadTaskResponse;
import xyz.suonan.myfolder_sever.transfer.error.TransferErrorCode;
import xyz.suonan.myfolder_sever.transfer.error.TransferException;
import xyz.suonan.myfolder_sever.transfer.model.UploadState;
import xyz.suonan.myfolder_sever.transfer.model.UploadTask;
import xyz.suonan.myfolder_sever.transfer.service.ResumableTransferService;
import xyz.suonan.myfolder_sever.transfer.service.TransferHash;
import xyz.suonan.myfolder_sever.transfer.store.JsonUploadTaskStore;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;
import static org.junit.jupiter.api.Assumptions.assumeTrue;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

class ResumableTransferServiceTest {
    private static final String USER = "alice";
    private static final int CHUNK_SIZE = 65_536;

    @TempDir
    Path temporaryDirectory;

    private Path storageRoot;
    private Path metadataRoot;
    private ObjectMapper objectMapper;
    private ResumableTransferService service;

    @BeforeEach
    void setUp() {
        storageRoot = temporaryDirectory.resolve("storage");
        metadataRoot = temporaryDirectory.resolve("metadata");
        objectMapper = new ObjectMapper().registerModule(new JavaTimeModule());
        service = newService();
    }

    @Test
    void persistsChunkProgressAcrossServiceRestart() {
        byte[] content = content(70_000);
        UploadTask task = service.create(USER, request(content));
        byte[] firstChunk = Arrays.copyOfRange(content, 0, CHUNK_SIZE);

        service.uploadChunk(USER, task.uploadId, "docs/data.bin", 0, 0, CHUNK_SIZE - 1,
                content.length, TransferHash.sha256(firstChunk), firstChunk);

        ResumableTransferService restartedService = newService();
        UploadTask restored = restartedService.status(USER, task.uploadId);
        UploadTaskResponse.FileStatus file = UploadTaskResponse.from(restored).files().get(0);
        assertEquals(List.of(0), file.completedChunks());
        assertEquals(List.of(1), file.missingChunks());
        assertEquals(UploadState.UPLOADING, restored.state);
        assertEquals(USER, restored.ownerUserId);
        assertFalse(objectMapper.valueToTree(UploadTaskResponse.from(restored)).has("ownerUserId"));
    }

    @Test
    void createsGroupUploadDirectlyAtGroupRoot() throws Exception {
        JsonUploadTaskStore store = new JsonUploadTaskStore(objectMapper, metadataRoot.toString());
        StorageScopeService scopes = mock(StorageScopeService.class);
        StorageQuotaService quota = mock(StorageQuotaService.class);
        Path groupRoot = temporaryDirectory.resolve("group-root");
        StorageScopeService.Scope scope = new StorageScopeService.Scope("GROUP", "group-1", groupRoot);
        when(scopes.resolve(eq(USER), eq("GROUP"), eq("group-1"), isNull(), eq(true)))
                .thenReturn(scope);
        ResumableTransferService scopedService = new ResumableTransferService(
                store, storageRoot.toString(), scopes, quota);
        String emptyHash = TransferHash.sha256(new byte[0]);
        CreateUploadTaskRequest request = new CreateUploadTaskRequest(
                "GROUP", "group-1", "", CHUNK_SIZE, 1, 0,
                List.of(new CreateUploadTaskRequest.FileManifest("root.txt", 0, emptyHash, 0)));

        UploadTask task = scopedService.create(USER, request);

        assertEquals("GROUP", task.scopeType);
        assertEquals("group-1", task.scopeId);
        assertEquals("", task.targetPath);
        assertTrue(Files.isRegularFile(groupRoot.resolve("root.txt")));
        verify(scopes).authorizePath(scope, USER, "", true);
        verify(scopes).authorizePath(scope, USER, "root.txt", true);
    }

    @Test
    void rejectsCorruptChunkWithoutAdvancingProgress() {
        byte[] content = content(70_000);
        UploadTask task = service.create(USER, request(content));
        byte[] firstChunk = Arrays.copyOfRange(content, 0, CHUNK_SIZE);

        TransferException exception = assertThrows(TransferException.class, () -> service.uploadChunk(USER, 
                task.uploadId, "docs/data.bin", 0, 0, CHUNK_SIZE - 1, content.length,
                "0".repeat(64), firstChunk));

        assertEquals(TransferErrorCode.CHUNK_HASH_MISMATCH, exception.getCode());
        assertEquals(List.of(0, 1), UploadTaskResponse.from(service.status(USER, task.uploadId))
                .files().get(0).missingChunks());
    }

    @Test
    void rejectsEveryUploadOperationFromAnotherUser() {
        byte[] content = content(70_000);
        UploadTask task = service.create(USER, request(content));
        byte[] firstChunk = Arrays.copyOfRange(content, 0, CHUNK_SIZE);

        assertTaskHidden(() -> service.status("bob", task.uploadId));
        assertTaskHidden(() -> service.uploadChunk("bob", task.uploadId, "docs/data.bin", 0,
                0, CHUNK_SIZE - 1, content.length, TransferHash.sha256(firstChunk), firstChunk));
        assertTaskHidden(() -> service.completeFile("bob", task.uploadId, "docs/data.bin"));
        assertTaskHidden(() -> service.completeTask("bob", task.uploadId));
        assertTaskHidden(() -> service.cancel("bob", task.uploadId));

        assertEquals(UploadState.PENDING, service.status(USER, task.uploadId).state);
    }

    @Test
    void completesFileAndTaskAfterSizeAndSha256Verification() throws Exception {
        byte[] content = content(70_000);
        UploadTask task = service.create(USER, request(content));
        byte[] firstChunk = Arrays.copyOfRange(content, 0, CHUNK_SIZE);
        byte[] secondChunk = Arrays.copyOfRange(content, CHUNK_SIZE, content.length);

        service.uploadChunk(USER, task.uploadId, "docs/data.bin", 0, 0, CHUNK_SIZE - 1,
                content.length, TransferHash.sha256(firstChunk), firstChunk);
        service.uploadChunk(USER, task.uploadId, "docs/data.bin", 1, CHUNK_SIZE, content.length - 1L,
                content.length, TransferHash.sha256(secondChunk), secondChunk);
        service.completeFile(USER, task.uploadId, "docs/data.bin");
        UploadTask completed = service.completeTask(USER, task.uploadId);

        assertEquals(UploadState.COMPLETED, completed.state);
        Path finalFile = storageRoot.resolve("inbox/MyFolder Upload/docs/data.bin");
        assertArrayEquals(content, Files.readAllBytes(finalFile));
        assertFalse(Files.exists(storageRoot.resolve(".myfolder/staging")
                .resolve(task.uploadId).resolve("docs/data.bin.part")));
    }

    @Test
    void reusesVerifiedContentWithoutUploadingAnyChunk() throws Exception {
        byte[] content = content(70_000);
        Path privateRoot = temporaryDirectory.resolve("private-alice");
        Files.createDirectories(privateRoot.resolve("existing"));
        Files.write(privateRoot.resolve("existing/source.bin"), content);
        JsonUploadTaskStore store = new JsonUploadTaskStore(objectMapper, metadataRoot.toString());
        StorageScopeService scopes = mock(StorageScopeService.class);
        StorageQuotaService quota = mock(StorageQuotaService.class);
        StorageScopeService.Scope scope = new StorageScopeService.Scope("PRIVATE", USER, privateRoot);
        when(scopes.resolve(eq(USER), eq("PRIVATE"), isNull(), isNull(), eq(true))).thenReturn(scope);
        ResumableTransferService scopedService = new ResumableTransferService(
                store, storageRoot.toString(), scopes, quota);

        UploadTask task = scopedService.create(USER, request(content));
        UploadTaskResponse.FileStatus status = UploadTaskResponse.from(task).files().get(0);

        assertEquals(UploadState.COMPLETED, status.state());
        assertTrue(status.missingChunks().isEmpty());
        assertArrayEquals(content, Files.readAllBytes(
                privateRoot.resolve("inbox/MyFolder Upload/docs/data.bin")));
        assertEquals(UploadState.COMPLETED, scopedService.completeTask(USER, task.uploadId).state);
    }

    @Test
    void writesDirectlyToRootAndSingleSegmentTargets() throws Exception {
        byte[] content = content(1_024);
        for (String targetPath : List.of("", "documents")) {
            String filePath = targetPath.isEmpty() ? "root.bin" : "nested.bin";
            CreateUploadTaskRequest direct = new CreateUploadTaskRequest(targetPath, CHUNK_SIZE, 1,
                    content.length, List.of(new CreateUploadTaskRequest.FileManifest(filePath,
                    content.length, TransferHash.sha256(content), 1)));
            UploadTask task = service.create(USER, direct);

            if (task.files.get(filePath).state != UploadState.COMPLETED) {
                service.uploadChunk(USER, task.uploadId, filePath, 0, 0, content.length - 1L,
                        content.length, TransferHash.sha256(content), content);
                service.completeFile(USER, task.uploadId, filePath);
            }
            service.completeTask(USER, task.uploadId);

            Path expected = targetPath.isEmpty()
                    ? storageRoot.resolve(filePath)
                    : storageRoot.resolve(targetPath).resolve(filePath);
            assertArrayEquals(content, Files.readAllBytes(expected));
        }
    }

    @Test
    void rejectsTraversalInTargetPath() {
        byte[] content = content(1_024);
        CreateUploadTaskRequest unsafe = new CreateUploadTaskRequest("../outside", CHUNK_SIZE, 1,
                content.length, List.of(new CreateUploadTaskRequest.FileManifest("safe.bin",
                content.length, TransferHash.sha256(content), 1)));

        TransferException exception = assertThrows(TransferException.class, () -> service.create(USER, unsafe));

        assertEquals(TransferErrorCode.INVALID_PATH, exception.getCode());
        assertFalse(Files.exists(temporaryDirectory.resolve("outside")));
    }

    @Test
    void rejectsTraversalBeforeWritingAnything() {
        byte[] content = content(70_000);
        CreateUploadTaskRequest unsafe = new CreateUploadTaskRequest("inbox/MyFolder Upload",
                CHUNK_SIZE, 1, content.length, List.of(new CreateUploadTaskRequest.FileManifest(
                "../outside.bin", content.length, TransferHash.sha256(content), 2)));

        TransferException exception = assertThrows(TransferException.class, () -> service.create(USER, unsafe));

        assertEquals(TransferErrorCode.INVALID_PATH, exception.getCode());
        assertFalse(Files.exists(temporaryDirectory.resolve("outside.bin")));
    }

    @ParameterizedTest
    @ValueSource(strings = {
            "a/../b.bin",
            "a/./b.bin",
            "C:/outside.bin",
            "c:relative.bin",
            "/absolute.bin",
            "a//b.bin",
            "a/"
    })
    void rejectsEveryNonCanonicalOrPlatformSpecificPath(String unsafePath) {
        byte[] content = content(70_000);
        CreateUploadTaskRequest unsafe = new CreateUploadTaskRequest("inbox/MyFolder Upload",
                CHUNK_SIZE, 1, content.length, List.of(new CreateUploadTaskRequest.FileManifest(
                unsafePath, content.length, TransferHash.sha256(content), 2)));

        TransferException exception = assertThrows(TransferException.class, () -> service.create(USER, unsafe));

        assertEquals(TransferErrorCode.INVALID_PATH, exception.getCode());
    }

    @Test
    void finalHashMismatchResetsTheWholeFileForRecovery() {
        byte[] content = content(70_000);
        CreateUploadTaskRequest wrongManifest = new CreateUploadTaskRequest("inbox/MyFolder Upload",
                CHUNK_SIZE, 1, content.length, List.of(new CreateUploadTaskRequest.FileManifest(
                "docs/data.bin", content.length, "f".repeat(64), 2)));
        UploadTask task = service.create(USER, wrongManifest);
        byte[] firstChunk = Arrays.copyOfRange(content, 0, CHUNK_SIZE);
        byte[] secondChunk = Arrays.copyOfRange(content, CHUNK_SIZE, content.length);
        service.uploadChunk(USER, task.uploadId, "docs/data.bin", 0, 0, CHUNK_SIZE - 1,
                content.length, TransferHash.sha256(firstChunk), firstChunk);
        service.uploadChunk(USER, task.uploadId, "docs/data.bin", 1, CHUNK_SIZE, content.length - 1L,
                content.length, TransferHash.sha256(secondChunk), secondChunk);

        TransferException exception = assertThrows(TransferException.class,
                () -> service.completeFile(USER, task.uploadId, "docs/data.bin"));

        assertEquals(TransferErrorCode.FILE_HASH_MISMATCH, exception.getCode());
        UploadTaskResponse.FileStatus status = UploadTaskResponse.from(service.status(USER, task.uploadId)).files().get(0);
        assertEquals(UploadState.FAILED, status.state());
        assertEquals(List.of(0, 1), status.missingChunks());
    }

    @Test
    void cancellationRemovesOnlyFilesCreatedByTheTask() throws Exception {
        byte[] content = content(70_000);
        UploadTask task = service.create(USER, request(content));
        byte[] firstChunk = Arrays.copyOfRange(content, 0, CHUNK_SIZE);
        byte[] secondChunk = Arrays.copyOfRange(content, CHUNK_SIZE, content.length);
        service.uploadChunk(USER, task.uploadId, "docs/data.bin", 0, 0, CHUNK_SIZE - 1,
                content.length, TransferHash.sha256(firstChunk), firstChunk);
        service.uploadChunk(USER, task.uploadId, "docs/data.bin", 1, CHUNK_SIZE, content.length - 1L,
                content.length, TransferHash.sha256(secondChunk), secondChunk);
        service.completeFile(USER, task.uploadId, "docs/data.bin");
        Path finalFile = storageRoot.resolve("inbox/MyFolder Upload/docs/data.bin");
        assertTrue(Files.exists(finalFile));

        UploadTask cancelled = service.cancel(USER, task.uploadId);

        assertEquals(UploadState.CANCELLED, cancelled.state);
        assertFalse(Files.exists(finalFile));
    }

    @Test
    void rejectsExistingSymbolicLinkInsideStoragePath() throws Exception {
        Files.createDirectories(storageRoot);
        Path outside = temporaryDirectory.resolve("outside");
        Files.createDirectories(outside);
        try {
            Files.createSymbolicLink(storageRoot.resolve("inbox"), outside);
        } catch (UnsupportedOperationException | java.nio.file.FileSystemException exception) {
            assumeTrue(false, "Symbolic links are unavailable in this test environment");
        }
        String emptyHash = TransferHash.sha256(new byte[0]);
        CreateUploadTaskRequest unsafe = new CreateUploadTaskRequest("inbox/MyFolder Upload",
                CHUNK_SIZE, 1, 0, List.of(new CreateUploadTaskRequest.FileManifest(
                "docs/empty.txt", 0, emptyHash, 0)));

        TransferException exception = assertThrows(TransferException.class, () -> service.create(USER, unsafe));

        assertEquals(TransferErrorCode.INVALID_PATH, exception.getCode());
        assertFalse(Files.exists(outside.resolve("MyFolder Upload/docs/empty.txt")));
    }

    private ResumableTransferService newService() {
        JsonUploadTaskStore store = new JsonUploadTaskStore(objectMapper, metadataRoot.toString());
        return new ResumableTransferService(store, storageRoot.toString());
    }

    private CreateUploadTaskRequest request(byte[] content) {
        return new CreateUploadTaskRequest("inbox/MyFolder Upload", CHUNK_SIZE, 1,
                content.length, List.of(new CreateUploadTaskRequest.FileManifest("docs/data.bin",
                content.length, TransferHash.sha256(content), 2)));
    }

    private byte[] content(int size) {
        byte[] bytes = new byte[size];
        for (int index = 0; index < bytes.length; index++) {
            bytes[index] = (byte) (index % 251);
        }
        return bytes;
    }

    private void assertTaskHidden(org.junit.jupiter.api.function.Executable action) {
        TransferException exception = assertThrows(TransferException.class, action);
        assertEquals(TransferErrorCode.TASK_NOT_FOUND, exception.getCode());
    }
}
