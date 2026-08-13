package xyz.suonan.myfolder_sever.device.api;

import jakarta.servlet.http.HttpServletRequest;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.bind.MissingRequestHeaderException;
import org.springframework.web.bind.MissingServletRequestParameterException;
import org.springframework.web.method.annotation.MethodArgumentTypeMismatchException;
import xyz.suonan.myfolder_sever.device.service.DeviceService;
import xyz.suonan.myfolder_sever.forward.service.ForwardTaskService;

import java.time.Instant;
import java.util.Map;

@RestControllerAdvice(basePackages = {
        "xyz.suonan.myfolder_sever.device",
        "xyz.suonan.myfolder_sever.forward"
})
public class DeviceExceptionHandler {
    @ExceptionHandler({DeviceService.DeviceException.class, ForwardTaskService.ForwardException.class})
    public ResponseEntity<Map<String, Object>> handle(RuntimeException exception, HttpServletRequest request) {
        String code = exception instanceof DeviceService.DeviceException d ? d.getCode() : ((ForwardTaskService.ForwardException) exception).getCode();
        int status = switch (code) {
            case "DEVICE_NOT_FOUND", "FORWARD_NOT_FOUND", "FORWARD_FILE_NOT_FOUND" -> 404;
            case "TASK_STATE_CONFLICT", "FORWARD_INCOMPLETE", "RELAY_NOT_READY" -> 409;
            case "RANGE_NOT_SATISFIABLE" -> 416;
            case "UNAUTHORIZED" -> 401;
            default -> 400;
        };
        Map<String, Object> details = exception instanceof ForwardTaskService.ForwardException forward
                ? forward.getDetails() : Map.of();
        return ResponseEntity.status(HttpStatus.valueOf(status)).body(Map.of(
                "timestamp", Instant.now().toString(), "status", status, "code", code,
                "message", exception.getMessage(), "path", request.getRequestURI(), "details", details));
    }

    @ExceptionHandler({HttpMessageNotReadableException.class, MissingRequestHeaderException.class,
            MissingServletRequestParameterException.class,
            MethodArgumentTypeMismatchException.class})
    public ResponseEntity<Map<String, Object>> badRequest(Exception exception, HttpServletRequest request) {
        return ResponseEntity.badRequest().body(Map.of(
                "timestamp", Instant.now().toString(), "status", 400, "code", "INVALID_REQUEST",
                "message", exception.getMessage(), "path", request.getRequestURI(), "details", Map.of()));
    }

    @ExceptionHandler(IllegalStateException.class)
    public ResponseEntity<Map<String, Object>> storage(IllegalStateException exception, HttpServletRequest request) {
        return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(Map.of(
                "timestamp", Instant.now().toString(), "status", 500, "code", "STORAGE_ERROR",
                "message", exception.getMessage(), "path", request.getRequestURI(), "details", Map.of()));
    }
}
