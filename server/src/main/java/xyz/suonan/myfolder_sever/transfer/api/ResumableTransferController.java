package xyz.suonan.myfolder_sever.transfer.api;

import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import xyz.suonan.myfolder_sever.Config.OpenApiConfig;
import xyz.suonan.myfolder_sever.transfer.error.TransferErrorCode;
import xyz.suonan.myfolder_sever.transfer.error.TransferException;
import xyz.suonan.myfolder_sever.transfer.service.ResumableTransferService;

import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.List;

@RestController
@Tag(name = "Resumable uploads", description = "Chunked uploads with resumable status, per-chunk SHA-256 and whole-file SHA-256 verification.")
@SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
public class ResumableTransferController {
    private static final Pattern CONTENT_RANGE = Pattern.compile("bytes (\\d+)-(\\d+)/(\\d+)");
    private final ResumableTransferService service;

    public ResumableTransferController(ResumableTransferService service) {
        this.service = service;
    }

    @PostMapping("/file/uploadfile")
    public ResponseEntity<UploadTaskResponse> create(@RequestBody CreateUploadTaskRequest request,
                                                     HttpServletRequest servletRequest) {
        return ResponseEntity.status(HttpStatus.CREATED)
                .body(UploadTaskResponse.from(service.create(userId(servletRequest), request)));
    }

    @PostMapping("/api/v1/transfers/tasks")
    public ResponseEntity<UploadTaskResponse> createLegacy(@RequestBody LegacyCreateUploadTaskRequest request,
                                                           HttpServletRequest servletRequest) {
        return ResponseEntity.status(HttpStatus.CREATED)
                .body(UploadTaskResponse.from(service.create(userId(servletRequest), request.toCurrent())));
    }

    @GetMapping({"/file/uploadfile", "/api/v1/transfers/tasks"})
    public List<UploadTaskResponse> list(HttpServletRequest request) {
        return service.list(userId(request)).stream().map(UploadTaskResponse::from).toList();
    }

    @GetMapping({"/file/uploadfile/{uploadId}", "/api/v1/transfers/tasks/{uploadId}"})
    public UploadTaskResponse status(@PathVariable String uploadId, HttpServletRequest request) {
        return UploadTaskResponse.from(service.status(userId(request), uploadId));
    }

    @PutMapping({"/file/uploadfile/{uploadId}/chunks/{chunkIndex}",
            "/api/v1/transfers/tasks/{uploadId}/chunks/{chunkIndex}"})
    public UploadTaskResponse uploadChunk(@PathVariable String uploadId,
                                          @PathVariable int chunkIndex,
                                          @RequestHeader("X-File-Path") String encodedFilePath,
                                          @RequestHeader("X-Chunk-SHA256") String chunkSha256,
                                          @RequestHeader("Content-Range") String contentRange,
                                          @RequestBody byte[] content,
                                          HttpServletRequest request) {
        long[] range = parseRange(contentRange);
        String filePath = URLDecoder.decode(encodedFilePath, StandardCharsets.UTF_8);
        return UploadTaskResponse.from(service.uploadChunk(userId(request), uploadId, filePath, chunkIndex,
                range[0], range[1], range[2], chunkSha256, content));
    }

    @PostMapping({"/file/uploadfile/{uploadId}/files/complete",
            "/api/v1/transfers/tasks/{uploadId}/files/complete"})
    public UploadTaskResponse completeFile(@PathVariable String uploadId,
                                           @RequestBody CompleteFileRequest body,
                                           HttpServletRequest request) {
        if (body == null || body.filePath() == null) {
            throw new TransferException(TransferErrorCode.INVALID_REQUEST, HttpStatus.BAD_REQUEST,
                    "filePath is required");
        }
        return UploadTaskResponse.from(service.completeFile(userId(request), uploadId, body.filePath()));
    }

    @PostMapping({"/file/uploadfile/{uploadId}/complete",
            "/api/v1/transfers/tasks/{uploadId}/complete"})
    public UploadTaskResponse completeTask(@PathVariable String uploadId, HttpServletRequest request) {
        return UploadTaskResponse.from(service.completeTask(userId(request), uploadId));
    }

    @DeleteMapping({"/file/uploadfile/{uploadId}", "/api/v1/transfers/tasks/{uploadId}"})
    public UploadTaskResponse cancel(@PathVariable String uploadId, HttpServletRequest request) {
        return UploadTaskResponse.from(service.cancel(userId(request), uploadId));
    }

    private String userId(HttpServletRequest request) {
        Object value = request.getAttribute("myfolder.userId");
        if (value == null || value.toString().isBlank()) {
            throw new TransferException(TransferErrorCode.TASK_NOT_FOUND, HttpStatus.NOT_FOUND,
                    "Upload task not found");
        }
        return value.toString();
    }

    private long[] parseRange(String value) {
        Matcher matcher = CONTENT_RANGE.matcher(value == null ? "" : value);
        if (!matcher.matches()) {
            throw new TransferException(TransferErrorCode.CHUNK_RANGE_INVALID, HttpStatus.BAD_REQUEST,
                    "Content-Range must use: bytes start-end/total");
        }
        try {
            return new long[]{Long.parseLong(matcher.group(1)), Long.parseLong(matcher.group(2)),
                    Long.parseLong(matcher.group(3))};
        } catch (NumberFormatException exception) {
            throw new TransferException(TransferErrorCode.CHUNK_RANGE_INVALID, HttpStatus.BAD_REQUEST,
                    "Content-Range contains an invalid number");
        }
    }

    public record CompleteFileRequest(String filePath) {
    }

    public record LegacyCreateUploadTaskRequest(
            String directoryName,
            String parentPath,
            int chunkSize,
            int totalFiles,
            long totalBytes,
            java.util.List<CreateUploadTaskRequest.FileManifest> files
    ) {
        CreateUploadTaskRequest toCurrent() {
            String targetPath = directoryName == null || parentPath == null
                    ? null
                    : parentPath + "/" + directoryName;
            return new CreateUploadTaskRequest("PRIVATE", null, targetPath, chunkSize, totalFiles, totalBytes, files);
        }
    }
}
