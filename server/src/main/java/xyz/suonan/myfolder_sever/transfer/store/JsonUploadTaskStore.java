package xyz.suonan.myfolder_sever.transfer.store;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Repository;
import xyz.suonan.myfolder_sever.transfer.error.TransferErrorCode;
import xyz.suonan.myfolder_sever.transfer.error.TransferException;
import xyz.suonan.myfolder_sever.transfer.model.UploadTask;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Optional;

import static org.springframework.http.HttpStatus.INTERNAL_SERVER_ERROR;

@Repository
public class JsonUploadTaskStore implements UploadTaskStore {
    private final ObjectMapper objectMapper;
    private final Path metadataRoot;

    public JsonUploadTaskStore(ObjectMapper objectMapper,
                               @Value("${transfer.metadata-root:${basePath}/.myfolder/upload-tasks}") String metadataRoot) {
        this.objectMapper = objectMapper;
        this.metadataRoot = Path.of(metadataRoot).toAbsolutePath().normalize();
    }

    @Override
    public void save(UploadTask task) {
        try {
            Files.createDirectories(metadataRoot);
            Path destination = taskPath(task.uploadId);
            Path temporary = destination.resolveSibling(destination.getFileName() + ".tmp");
            objectMapper.writeValue(temporary.toFile(), task);
            try {
                Files.move(temporary, destination, StandardCopyOption.REPLACE_EXISTING,
                        StandardCopyOption.ATOMIC_MOVE);
            } catch (IOException atomicMoveUnsupported) {
                Files.move(temporary, destination, StandardCopyOption.REPLACE_EXISTING);
            }
        } catch (IOException exception) {
            throw storageError("Unable to persist upload task", exception);
        }
    }

    @Override
    public Optional<UploadTask> find(String uploadId) {
        Path path = taskPath(uploadId);
        if (!Files.exists(path)) {
            return Optional.empty();
        }
        try {
            return Optional.of(objectMapper.readValue(path.toFile(), UploadTask.class));
        } catch (IOException exception) {
            throw storageError("Unable to read upload task", exception);
        }
    }

    @Override
    public void delete(String uploadId) {
        try {
            Files.deleteIfExists(taskPath(uploadId));
        } catch (IOException exception) {
            throw storageError("Unable to delete upload task", exception);
        }
    }

    private Path taskPath(String uploadId) {
        if (uploadId == null || !uploadId.matches("[A-Z2-7]{26}")) {
            throw new TransferException(TransferErrorCode.INVALID_REQUEST,
                    org.springframework.http.HttpStatus.BAD_REQUEST, "Invalid uploadId");
        }
        return metadataRoot.resolve(uploadId + ".json");
    }

    private TransferException storageError(String message, IOException exception) {
        return new TransferException(TransferErrorCode.STORAGE_ERROR, INTERNAL_SERVER_ERROR,
                message + ": " + exception.getMessage());
    }
}
