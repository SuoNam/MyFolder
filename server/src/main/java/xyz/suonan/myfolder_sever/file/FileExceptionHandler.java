package xyz.suonan.myfolder_sever.file;

import jakarta.servlet.http.HttpServletRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

import java.time.Instant;
import java.util.Map;

@RestControllerAdvice(basePackages = "xyz.suonan.myfolder_sever.Controller")
public class FileExceptionHandler {
    private static final Logger log = LoggerFactory.getLogger(FileExceptionHandler.class);

    public record ErrorResponse(Instant timestamp, int status, String code, String message,
                                String path, Map<String, Object> details) {}

    @ExceptionHandler(FileOperationException.class)
    public ResponseEntity<ErrorResponse> handle(FileOperationException exception, HttpServletRequest request) {
        if (exception.getStatus().is5xxServerError()) {
            log.error("文件操作失败: code={}, path={}", exception.getCode(), request.getRequestURI(), exception);
        } else {
            log.warn("文件操作被拒绝: code={}, path={}, message={}", exception.getCode(),
                    request.getRequestURI(), exception.getMessage());
        }
        ErrorResponse response = new ErrorResponse(Instant.now(), exception.getStatus().value(),
                exception.getCode(), exception.getMessage(), request.getRequestURI(), exception.getDetails());
        return ResponseEntity.status(exception.getStatus()).body(response);
    }
}
