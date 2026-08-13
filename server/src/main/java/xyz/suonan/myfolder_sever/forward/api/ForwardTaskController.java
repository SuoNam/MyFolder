package xyz.suonan.myfolder_sever.forward.api;

import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import lombok.RequiredArgsConstructor;
import org.springframework.http.*;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.servlet.mvc.method.annotation.StreamingResponseBody;
import xyz.suonan.myfolder_sever.forward.model.ForwardTask;
import xyz.suonan.myfolder_sever.forward.service.ForwardTaskService;
import xyz.suonan.myfolder_sever.Config.OpenApiConfig;

import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.util.List;

@RestController
@RequestMapping("/api/v1/forwards")
@RequiredArgsConstructor
@Tag(name = "Forward tasks", description = "设备间转发任务。除 JWT 外还需要 X-Device-Id 和 X-Device-Token。")
@SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
public class ForwardTaskController {
    private final ForwardTaskService service;

    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public ForwardTask create(@RequestBody CreateForwardTaskRequest body,
                              @RequestHeader("X-Device-Id") String deviceId,
                              @RequestHeader("X-Device-Token") String deviceToken,
                              HttpServletRequest request) {
        return service.create(userId(request), deviceId, deviceToken, body);
    }

    @GetMapping
    public List<ForwardTask> list(@RequestHeader("X-Device-Id") String deviceId,
                                  @RequestHeader("X-Device-Token") String deviceToken,
                                  HttpServletRequest request) {
        return service.list(userId(request), deviceId, deviceToken);
    }

    @GetMapping("/{forwardId}")
    public ForwardTask get(@PathVariable String forwardId,
                           @RequestHeader("X-Device-Id") String deviceId,
                           @RequestHeader("X-Device-Token") String deviceToken,
                           HttpServletRequest request) {
        return service.get(userId(request), deviceId, deviceToken, forwardId);
    }

    @PostMapping("/{forwardId}/accept")
    public ForwardTask accept(@PathVariable String forwardId,
                              @RequestHeader("X-Device-Id") String deviceId,
                              @RequestHeader("X-Device-Token") String deviceToken,
                              HttpServletRequest request) {
        return service.accept(userId(request), deviceId, deviceToken, forwardId);
    }

    @PostMapping("/{forwardId}/start")
    public ForwardTask start(@PathVariable String forwardId,
                             @RequestHeader("X-Device-Id") String deviceId,
                             @RequestHeader("X-Device-Token") String deviceToken,
                             HttpServletRequest request) {
        return service.start(userId(request), deviceId, deviceToken, forwardId);
    }

    @PostMapping("/{forwardId}/progress")
    public ForwardTask progress(@PathVariable String forwardId,
                                @RequestBody ForwardProgressRequest body,
                                @RequestHeader("X-Device-Id") String deviceId,
                                @RequestHeader("X-Device-Token") String deviceToken,
                                HttpServletRequest request) {
        return service.progress(userId(request), deviceId, deviceToken, forwardId,
                body == null ? -1 : body.transferredBytes());
    }

    @PostMapping("/{forwardId}/complete")
    public ForwardTask complete(@PathVariable String forwardId,
                                @RequestHeader("X-Device-Id") String deviceId,
                                @RequestHeader("X-Device-Token") String deviceToken,
                                HttpServletRequest request) {
        return service.complete(userId(request), deviceId, deviceToken, forwardId);
    }

    @PostMapping("/{forwardId}/fail")
    public ForwardTask fail(@PathVariable String forwardId,
                            @RequestBody(required = false) ForwardFailureRequest body,
                            @RequestHeader("X-Device-Id") String deviceId,
                            @RequestHeader("X-Device-Token") String deviceToken,
                            HttpServletRequest request) {
        return service.fail(userId(request), deviceId, deviceToken, forwardId,
                body == null ? null : body.reason());
    }

    @DeleteMapping("/{forwardId}")
    public ForwardTask cancel(@PathVariable String forwardId,
                              @RequestHeader("X-Device-Id") String deviceId,
                              @RequestHeader("X-Device-Token") String deviceToken,
                              HttpServletRequest request) {
        return service.cancel(userId(request), deviceId, deviceToken, forwardId);
    }

    @GetMapping("/{forwardId}/files/content")
    public ResponseEntity<StreamingResponseBody> content(
            @PathVariable String forwardId,
            @RequestParam("path") String path,
            @RequestHeader(value = HttpHeaders.RANGE, required = false) String range,
            @RequestHeader("X-Device-Id") String deviceId,
            @RequestHeader("X-Device-Token") String deviceToken,
            HttpServletRequest request) {
        ForwardTaskService.RelayContent content = service.relayContent(
                userId(request), deviceId, deviceToken, forwardId, path, range);
        StreamingResponseBody body = output -> {
            try (RandomAccessFile input = new RandomAccessFile(content.path().toFile(), "r")) {
                input.seek(content.start());
                long remaining = content.contentLength();
                byte[] buffer = new byte[64 * 1024];
                while (remaining > 0) {
                    int read = input.read(buffer, 0, (int) Math.min(buffer.length, remaining));
                    if (read < 0) break;
                    output.write(buffer, 0, read);
                    remaining -= read;
                }
            }
        };

        HttpHeaders headers = new HttpHeaders();
        headers.setContentType(MediaType.APPLICATION_OCTET_STREAM);
        headers.set(HttpHeaders.ACCEPT_RANGES, "bytes");
        headers.set("X-File-SHA256", content.sha256());
        headers.setContentLength(content.contentLength());
        headers.setContentDisposition(ContentDisposition.attachment()
                .filename(fileName(content.filePath()), StandardCharsets.UTF_8).build());
        if (content.partial()) {
            headers.set(HttpHeaders.CONTENT_RANGE,
                    "bytes " + content.start() + "-" + content.end() + "/" + content.fileSize());
        }
        return new ResponseEntity<>(body, headers, content.partial() ? HttpStatus.PARTIAL_CONTENT : HttpStatus.OK);
    }

    private String fileName(String path) {
        int separator = path.lastIndexOf('/');
        return separator < 0 ? path : path.substring(separator + 1);
    }

    private String userId(HttpServletRequest request) {
        Object value = request.getAttribute("myfolder.userId");
        if (value == null) throw new ForwardTaskService.ForwardException("UNAUTHORIZED", "Authenticated user is required");
        return value.toString();
    }
}
