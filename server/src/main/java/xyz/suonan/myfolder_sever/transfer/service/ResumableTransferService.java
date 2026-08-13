package xyz.suonan.myfolder_sever.transfer.service;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.beans.factory.annotation.Autowired;
import xyz.suonan.myfolder_sever.Utils.IdGen;
import xyz.suonan.myfolder_sever.Utils.SafeRelativePath;
import xyz.suonan.myfolder_sever.transfer.api.CreateUploadTaskRequest;
import xyz.suonan.myfolder_sever.transfer.error.TransferErrorCode;
import xyz.suonan.myfolder_sever.transfer.error.TransferException;
import xyz.suonan.myfolder_sever.transfer.model.UploadFile;
import xyz.suonan.myfolder_sever.transfer.model.UploadState;
import xyz.suonan.myfolder_sever.transfer.model.UploadTask;
import xyz.suonan.myfolder_sever.transfer.store.UploadTaskStore;
import xyz.suonan.myfolder_sever.file.StorageScopeService;
import xyz.suonan.myfolder_sever.file.StorageQuotaService;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import static org.springframework.http.HttpStatus.*;

@Service
public class ResumableTransferService {
    private static final int MIN_CHUNK_SIZE = 64 * 1024;
    private static final int MAX_CHUNK_SIZE = 64 * 1024 * 1024;
    private static final String SHA_256_PATTERN = "(?i)[0-9a-f]{64}";

    private final UploadTaskStore store;
    private final Path storageRoot;
    private final StorageScopeService scopes;
    private final StorageQuotaService quota;
    private final Map<String, Object> taskLocks = new ConcurrentHashMap<>();

    @Autowired
    public ResumableTransferService(UploadTaskStore store,
                                    @Value("${transfer.storage-root:${basePath}}") String storageRoot,
                                    StorageScopeService scopes, StorageQuotaService quota) {
        this.store = store;
        this.storageRoot = Path.of(storageRoot).toAbsolutePath().normalize();
        this.scopes = scopes;
        this.quota = quota;
    }

    /** Unit-test/embedded compatibility constructor. Production always injects scoped storage. */
    public ResumableTransferService(UploadTaskStore store, String storageRoot) {
        this.store = store;
        this.storageRoot = Path.of(storageRoot).toAbsolutePath().normalize();
        this.scopes = null;
        this.quota = null;
    }

    public UploadTask create(String ownerUserId, CreateUploadTaskRequest request) {
        requireUser(ownerUserId);
        validateManifest(request);
        UploadTask task = new UploadTask();
        task.uploadId = IdGen.generateId();
        task.ownerUserId = ownerUserId;
        task.scopeType = request.scopeType() == null || request.scopeType().isBlank()
                ? "PRIVATE" : request.scopeType().trim().toUpperCase(java.util.Locale.ROOT);
        task.scopeId = request.scopeId();
        if (scopes != null) {
            var scope = scopes.resolve(ownerUserId, task.scopeType, task.scopeId, null, true);
            scopes.authorizePath(scope, ownerUserId, request.targetPath(), true);
        }
        task.targetPath = normalizeTargetPath(request.targetPath());
        task.chunkSize = request.chunkSize();
        task.totalFiles = request.totalFiles();
        task.totalBytes = request.totalBytes();
        task.createdAt = Instant.now();
        task.updatedAt = task.createdAt;
        for (CreateUploadTaskRequest.FileManifest manifest : request.files()) {
            String path = normalizeRelativePath(manifest.path(), "file path");
            if (task.files.containsKey(path)) {
                throw invalid("Duplicate file path: " + path);
            }
            UploadFile file = new UploadFile(path, manifest.size(), manifest.sha256().toLowerCase(),
                    manifest.totalChunks());
            if (file.size == 0) {
                String emptyHash = TransferHash.sha256(new byte[0]);
                if (!emptyHash.equals(file.sha256)) {
                    throw invalid("Empty file SHA-256 is inconsistent for " + path);
                }
                file.state = UploadState.COMPLETED;
            }
            task.files.put(path, file);
        }
        if (quota != null) quota.reserve(task.uploadId, ownerUserId, request.totalBytes());
        try {
            store.save(task);
            createEmptyFiles(task);
            return task;
        } catch (RuntimeException exception) {
            if (quota != null) quota.release(task.uploadId);
            throw exception;
        }
    }

    public UploadTask status(String ownerUserId, String uploadId) {
        return loadOwned(ownerUserId, uploadId);
    }

    public UploadTask uploadChunk(String ownerUserId, String uploadId, String filePath, int chunkIndex,
                                  long rangeStart, long rangeEnd, long rangeTotal,
                                  String chunkSha256, byte[] content) {
        Object lock = taskLocks.computeIfAbsent(uploadId, ignored -> new Object());
        synchronized (lock) {
            UploadTask task = loadOwned(ownerUserId, uploadId);
            ensureMutable(task);
            String normalizedPath = normalizeRelativePath(filePath, "file path");
            UploadFile file = requireFile(task, normalizedPath);
            validateChunk(task, file, chunkIndex, rangeStart, rangeEnd, rangeTotal, chunkSha256, content);

            String actualChunkHash = TransferHash.sha256(content);
            if (!actualChunkHash.equalsIgnoreCase(chunkSha256)) {
                throw new TransferException(TransferErrorCode.CHUNK_HASH_MISMATCH, UNPROCESSABLE_ENTITY,
                        "Chunk SHA-256 mismatch", Map.of("expected", chunkSha256, "actual", actualChunkHash,
                                "chunkIndex", chunkIndex));
            }
            try {
                Path staging = stagingPath(task, file);
                Files.createDirectories(staging.getParent());
                try (RandomAccessFile output = new RandomAccessFile(staging.toFile(), "rw")) {
                    output.seek(rangeStart);
                    output.write(content);
                    output.getFD().sync();
                }
            } catch (IOException exception) {
                throw storage("Unable to write chunk", exception);
            }

            file.completedChunks.add(chunkIndex);
            file.failureReason = null;
            file.state = UploadState.UPLOADING;
            task.state = UploadState.UPLOADING;
            touch(task);
            store.save(task);
            return task;
        }
    }

    public UploadTask completeFile(String ownerUserId, String uploadId, String filePath) {
        Object lock = taskLocks.computeIfAbsent(uploadId, ignored -> new Object());
        synchronized (lock) {
            UploadTask task = loadOwned(ownerUserId, uploadId);
            ensureMutable(task);
            UploadFile file = requireFile(task, normalizeRelativePath(filePath, "file path"));
            List<Integer> missing = missingChunks(file);
            if (!missing.isEmpty()) {
                throw new TransferException(TransferErrorCode.FILE_INCOMPLETE, CONFLICT,
                        "File still has missing chunks", Map.of("missingChunks", missing));
            }

            file.state = UploadState.VERIFYING;
            task.state = UploadState.VERIFYING;
            touch(task);
            store.save(task);
            Path staging = stagingPath(task, file);
            try {
                long actualSize = Files.size(staging);
                if (actualSize != file.size) {
                    resetCorruptFile(task, file, "Expected " + file.size + " bytes but found " + actualSize);
                    throw new TransferException(TransferErrorCode.FILE_SIZE_MISMATCH, UNPROCESSABLE_ENTITY,
                            "File size mismatch", Map.of("expected", file.size, "actual", actualSize));
                }
                String actualHash = TransferHash.sha256(staging);
                if (!actualHash.equalsIgnoreCase(file.sha256)) {
                    resetCorruptFile(task, file, "Expected " + file.sha256 + " but found " + actualHash);
                    throw new TransferException(TransferErrorCode.FILE_HASH_MISMATCH, UNPROCESSABLE_ENTITY,
                            "File SHA-256 mismatch", Map.of("expected", file.sha256, "actual", actualHash));
                }
                Path target = finalPath(task, file);
                Files.createDirectories(target.getParent());
                if (Files.exists(target)) {
                    if (Files.size(target) == file.size && TransferHash.sha256(target).equalsIgnoreCase(file.sha256)) {
                        Files.deleteIfExists(staging);
                        file.materializedByTask = false;
                    } else {
                        throw new TransferException(TransferErrorCode.TARGET_ALREADY_EXISTS, CONFLICT,
                                "Target file already exists with different content",
                                Map.of("filePath", file.path));
                    }
                } else {
                    moveAtomically(staging, target);
                    file.materializedByTask = true;
                }
                if (quota != null && "GROUP".equals(task.scopeType)) {
                    String objectPath = task.targetPath == null || task.targetPath.isBlank()
                            ? file.path : task.targetPath + "/" + file.path;
                    quota.recordGroupFile(task.scopeId, objectPath, task.ownerUserId, file.size);
                }
                if (quota != null) quota.consume(task.uploadId, file.size);
            } catch (TransferException exception) {
                throw exception;
            } catch (IOException exception) {
                throw storage("Unable to verify or finalize file", exception);
            }

            file.state = UploadState.COMPLETED;
            file.failureReason = null;
            task.state = allFilesComplete(task) ? UploadState.VERIFYING : UploadState.UPLOADING;
            touch(task);
            store.save(task);
            return task;
        }
    }

    public UploadTask completeTask(String ownerUserId, String uploadId) {
        Object lock = taskLocks.computeIfAbsent(uploadId, ignored -> new Object());
        synchronized (lock) {
            UploadTask task = loadOwned(ownerUserId, uploadId);
            if (task.state == UploadState.COMPLETED) {
                return task;
            }
            ensureMutable(task);
            List<String> incompleteFiles = task.files.values().stream()
                    .filter(file -> file.state != UploadState.COMPLETED)
                    .map(file -> file.path)
                    .toList();
            if (!incompleteFiles.isEmpty()) {
                throw new TransferException(TransferErrorCode.FILE_INCOMPLETE, CONFLICT,
                        "Upload task still has incomplete files", Map.of("files", incompleteFiles));
            }
            task.state = UploadState.COMPLETED;
            task.failureReason = null;
            touch(task);
            store.save(task);
            if (quota != null) quota.release(task.uploadId);
            return task;
        }
    }

    public UploadTask cancel(String ownerUserId, String uploadId) {
        Object lock = taskLocks.computeIfAbsent(uploadId, ignored -> new Object());
        synchronized (lock) {
            UploadTask task = loadOwned(ownerUserId, uploadId);
            if (task.state == UploadState.COMPLETED) {
                throw new TransferException(TransferErrorCode.TASK_STATE_CONFLICT, CONFLICT,
                        "Completed upload cannot be cancelled");
            }
            task.state = UploadState.CANCELLED;
            touch(task);
            store.save(task);
            deleteStagingDirectory(task);
            deleteMaterializedFiles(task);
            if (quota != null) quota.release(task.uploadId);
            return task;
        }
    }

    private void validateManifest(CreateUploadTaskRequest request) {
        if (request == null || request.files() == null || request.targetPath() == null) {
            throw invalid("targetPath and files are required");
        }
        if (request.chunkSize() < MIN_CHUNK_SIZE || request.chunkSize() > MAX_CHUNK_SIZE) {
            throw invalid("chunkSize must be between 65536 and 67108864 bytes");
        }
        if (request.totalFiles() != request.files().size() || request.totalFiles() < 0) {
            throw invalid("totalFiles does not match files manifest");
        }
        long totalBytes = 0;
        for (CreateUploadTaskRequest.FileManifest file : request.files()) {
            if (file == null || file.path() == null || file.sha256() == null || file.size() < 0
                    || !file.sha256().matches(SHA_256_PATTERN)) {
                throw invalid("Each file requires a safe path, non-negative size and SHA-256");
            }
            int expectedChunks = file.size() == 0 ? 0 : (int) ((file.size() + request.chunkSize() - 1) / request.chunkSize());
            if (file.totalChunks() != expectedChunks) {
                throw invalid("totalChunks is inconsistent for " + file.path());
            }
            try {
                totalBytes = Math.addExact(totalBytes, file.size());
            } catch (ArithmeticException exception) {
                throw invalid("totalBytes overflow");
            }
        }
        if (totalBytes != request.totalBytes()) {
            throw invalid("totalBytes does not match files manifest");
        }
    }

    private void validateChunk(UploadTask task, UploadFile file, int chunkIndex,
                               long rangeStart, long rangeEnd, long rangeTotal,
                               String chunkSha256, byte[] content) {
        if (file.state == UploadState.COMPLETED) {
            throw new TransferException(TransferErrorCode.TASK_STATE_CONFLICT, CONFLICT,
                    "File is already completed");
        }
        if (chunkIndex < 0 || chunkIndex >= file.totalChunks) {
            throw new TransferException(TransferErrorCode.CHUNK_INDEX_INVALID, BAD_REQUEST,
                    "Chunk index is outside the file manifest");
        }
        long expectedStart = (long) chunkIndex * task.chunkSize;
        long expectedEnd = Math.min(file.size, expectedStart + task.chunkSize) - 1;
        if (rangeStart != expectedStart || rangeEnd != expectedEnd || rangeTotal != file.size) {
            throw new TransferException(TransferErrorCode.CHUNK_RANGE_INVALID, BAD_REQUEST,
                    "Content-Range does not match the manifest", Map.of("expected",
                    "bytes " + expectedStart + "-" + expectedEnd + "/" + file.size));
        }
        long expectedLength = expectedEnd - expectedStart + 1;
        if (content == null || content.length != expectedLength) {
            throw new TransferException(TransferErrorCode.CHUNK_SIZE_INVALID, BAD_REQUEST,
                    "Chunk body length does not match Content-Range", Map.of("expected", expectedLength,
                    "actual", content == null ? 0 : content.length));
        }
        if (chunkSha256 == null || !chunkSha256.matches(SHA_256_PATTERN)) {
            throw invalid("X-Chunk-SHA256 must be a 64-character hexadecimal SHA-256");
        }
    }

    public UploadTask completedForRelay(String ownerUserId, String uploadId) {
        UploadTask task = loadOwned(ownerUserId, uploadId);
        if (task.state != UploadState.COMPLETED) {
            throw new TransferException(TransferErrorCode.TASK_STATE_CONFLICT, CONFLICT,
                    "Upload task is not completed");
        }
        return task;
    }

    public Path completedFilePath(String ownerUserId, String uploadId, String filePath) {
        UploadTask task = completedForRelay(ownerUserId, uploadId);
        UploadFile file = requireFile(task, normalizeRelativePath(filePath, "file path"));
        if (file.state != UploadState.COMPLETED) {
            throw new TransferException(TransferErrorCode.FILE_NOT_FOUND, NOT_FOUND,
                    "Completed file is not available");
        }
        Path path = finalPath(task, file);
        if (!Files.isRegularFile(path, java.nio.file.LinkOption.NOFOLLOW_LINKS)) {
            throw new TransferException(TransferErrorCode.FILE_NOT_FOUND, NOT_FOUND,
                    "Completed file is not available");
        }
        return path;
    }

    private UploadTask loadOwned(String ownerUserId, String uploadId) {
        requireUser(ownerUserId);
        UploadTask task = store.find(uploadId).orElseThrow(() -> taskNotFound());
        if (task.ownerUserId == null || !task.ownerUserId.equals(ownerUserId)) {
            throw taskNotFound();
        }
        migrateLegacyTargetPath(task);
        return task;
    }

    private TransferException taskNotFound() {
        return new TransferException(TransferErrorCode.TASK_NOT_FOUND, NOT_FOUND, "Upload task not found");
    }

    private void requireUser(String ownerUserId) {
        if (ownerUserId == null || ownerUserId.isBlank()) {
            throw taskNotFound();
        }
    }

    private UploadFile requireFile(UploadTask task, String path) {
        UploadFile file = task.files.get(path);
        if (file == null) {
            throw new TransferException(TransferErrorCode.FILE_NOT_FOUND, NOT_FOUND,
                    "File is not part of the upload manifest");
        }
        return file;
    }

    private void ensureMutable(UploadTask task) {
        if (task.state == UploadState.COMPLETED || task.state == UploadState.CANCELLED) {
            throw new TransferException(TransferErrorCode.TASK_STATE_CONFLICT, CONFLICT,
                    "Upload task is " + task.state);
        }
    }

    private String normalizeRelativePath(String value, String fieldName) {
        try {
            return SafeRelativePath.normalize(value);
        } catch (IllegalArgumentException exception) {
            throw new TransferException(TransferErrorCode.INVALID_PATH, BAD_REQUEST,
                    fieldName + " must be a safe relative path");
        }
    }

    private String normalizeTargetPath(String value) {
        if (value != null && value.isEmpty()) {
            return "";
        }
        return normalizeRelativePath(value, "targetPath");
    }

    private void migrateLegacyTargetPath(UploadTask task) {
        if (task.scopeType == null || task.scopeType.isBlank()) task.scopeType = "PRIVATE";
        if (task.targetPath != null) {
            return;
        }
        if (task.parentPath == null || task.directoryName == null) {
            throw new TransferException(TransferErrorCode.INVALID_PATH, BAD_REQUEST,
                    "Upload task has no valid targetPath");
        }
        task.targetPath = normalizeRelativePath(task.parentPath + "/" + task.directoryName, "targetPath");
    }

    private Path finalPath(UploadTask task, UploadFile file) {
        Path scopeRoot = scopes == null ? storageRoot
                : scopes.resolve(task.ownerUserId, task.scopeType, task.scopeId, null, true).root();
        if (scopes != null) scopes.authorizePath(
                new StorageScopeService.Scope(task.scopeType, task.scopeId, scopeRoot),
                task.ownerUserId, (task.targetPath == null ? "" : task.targetPath) + "/" + file.path, true);
        return resolveInside(scopeRoot, task.targetPath, file.path);
    }

    private Path stagingPath(UploadTask task, UploadFile file) {
        Path stagingRoot = storageRoot.resolve(".myfolder/staging").normalize();
        return resolveInside(stagingRoot, task.uploadId, file.path + ".part");
    }

    private Path resolveInside(Path root, String... paths) {
        Path normalizedRoot = root.toAbsolutePath().normalize();
        Path result = normalizedRoot;
        for (String path : paths) {
            result = result.resolve(path);
        }
        result = result.toAbsolutePath().normalize();
        if (!result.startsWith(normalizedRoot)) {
            throw new TransferException(TransferErrorCode.INVALID_PATH, BAD_REQUEST,
                    "Resolved path escapes the transfer root");
        }
        Path cursor = normalizedRoot;
        for (Path segment : normalizedRoot.relativize(result)) {
            cursor = cursor.resolve(segment);
            if (Files.exists(cursor, java.nio.file.LinkOption.NOFOLLOW_LINKS)
                    && Files.isSymbolicLink(cursor)) {
                throw new TransferException(TransferErrorCode.INVALID_PATH, BAD_REQUEST,
                        "Symbolic links are not allowed inside the transfer path");
            }
        }
        return result;
    }

    private void createEmptyFiles(UploadTask task) {
        try {
            for (UploadFile file : task.files.values()) {
                if (file.size == 0) {
                    Path target = finalPath(task, file);
                    Files.createDirectories(target.getParent());
                    if (!Files.exists(target)) {
                        Files.createFile(target);
                        file.materializedByTask = true;
                    } else if (Files.size(target) != 0) {
                        throw new TransferException(TransferErrorCode.TARGET_ALREADY_EXISTS, CONFLICT,
                                "Target file already exists with different content", Map.of("filePath", file.path));
                    }
                }
            }
        } catch (IOException exception) {
            task.state = UploadState.FAILED;
            task.failureReason = "Unable to create empty file: " + exception.getMessage();
            touch(task);
            store.save(task);
            throw storage("Unable to create empty file", exception);
        }
    }

    private void resetCorruptFile(UploadTask task, UploadFile file, String reason) throws IOException {
        Files.deleteIfExists(stagingPath(task, file));
        file.completedChunks.clear();
        file.state = UploadState.FAILED;
        file.failureReason = reason;
        task.state = UploadState.FAILED;
        task.failureReason = "File verification failed: " + file.path;
        touch(task);
        store.save(task);
    }

    private void deleteStagingDirectory(UploadTask task) {
        Path directory = storageRoot.resolve(".myfolder/staging").resolve(task.uploadId).normalize();
        if (!directory.startsWith(storageRoot.resolve(".myfolder/staging").normalize()) || !Files.exists(directory)) {
            return;
        }
        try (var paths = Files.walk(directory)) {
            paths.sorted(java.util.Comparator.reverseOrder()).forEach(path -> {
                try {
                    Files.deleteIfExists(path);
                } catch (IOException ignored) {
                    // Cancellation is persisted even when best-effort staging cleanup fails.
                }
            });
        } catch (IOException ignored) {
            // Cancellation is persisted even when best-effort staging cleanup fails.
        }
    }

    private void deleteMaterializedFiles(UploadTask task) {
        for (UploadFile file : task.files.values()) {
            if (!file.materializedByTask) {
                continue;
            }
            Path target = finalPath(task, file);
            try {
                if (Files.exists(target) && Files.size(target) == file.size
                        && TransferHash.sha256(target).equalsIgnoreCase(file.sha256)) {
                    Files.delete(target);
                    file.materializedByTask = false;
                }
            } catch (IOException ignored) {
                // Cancellation remains durable; cleanup can be retried by an operator.
            }
        }
        touch(task);
        store.save(task);
    }

    private List<Integer> missingChunks(UploadFile file) {
        List<Integer> missing = new ArrayList<>();
        for (int index = 0; index < file.totalChunks; index++) {
            if (!file.completedChunks.contains(index)) {
                missing.add(index);
            }
        }
        return missing;
    }

    private boolean allFilesComplete(UploadTask task) {
        return task.files.values().stream().allMatch(file -> file.state == UploadState.COMPLETED);
    }

    private void moveAtomically(Path source, Path target) throws IOException {
        try {
            Files.move(source, target, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
        } catch (IOException atomicMoveUnsupported) {
            Files.move(source, target, StandardCopyOption.REPLACE_EXISTING);
        }
    }

    private void touch(UploadTask task) {
        task.updatedAt = Instant.now();
    }

    private TransferException invalid(String message) {
        return new TransferException(TransferErrorCode.INVALID_REQUEST, BAD_REQUEST, message);
    }

    private TransferException storage(String message, IOException exception) {
        return new TransferException(TransferErrorCode.STORAGE_ERROR, HttpStatus.INTERNAL_SERVER_ERROR,
                message + ": " + exception.getMessage());
    }
}
