package xyz.suonan.myfolder_sever.auth;

import jakarta.servlet.http.HttpServletRequest;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

import java.time.Instant;
import java.util.Map;

@RestControllerAdvice
public class AuthExceptionHandler {
    @ExceptionHandler(AuthException.class)
    public ResponseEntity<Map<String, Object>> auth(AuthException error, HttpServletRequest request) {
        return ResponseEntity.status(error.status()).body(body(error.status(), error.code(), error.getMessage(), request));
    }

    @ExceptionHandler(DataIntegrityViolationException.class)
    public ResponseEntity<Map<String, Object>> conflict(DataIntegrityViolationException error, HttpServletRequest request) {
        return ResponseEntity.status(HttpStatus.CONFLICT)
                .body(body(HttpStatus.CONFLICT, "ACCOUNT_OR_EMAIL_EXISTS", "账号或邮箱已被使用", request));
    }

    private Map<String, Object> body(HttpStatus status, String code, String message, HttpServletRequest request) {
        return Map.of("timestamp", Instant.now().toString(), "status", status.value(), "code", code,
                "message", message, "path", request.getRequestURI(), "details", Map.of());
    }
}
