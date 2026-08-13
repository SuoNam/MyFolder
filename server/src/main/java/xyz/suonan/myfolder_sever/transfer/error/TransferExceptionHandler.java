package xyz.suonan.myfolder_sever.transfer.error;

import jakarta.servlet.http.HttpServletRequest;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.bind.MissingRequestHeaderException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;
import org.springframework.web.method.annotation.MethodArgumentTypeMismatchException;

import java.time.Instant;
import java.util.Map;

@RestControllerAdvice(basePackages = "xyz.suonan.myfolder_sever.transfer")
public class TransferExceptionHandler {
    public record ErrorResponse(
            Instant timestamp,
            int status,
            TransferErrorCode code,
            String message,
            String path,
            Map<String, Object> details
    ) {
    }

    @ExceptionHandler(TransferException.class)
    public ResponseEntity<ErrorResponse> handle(TransferException exception, HttpServletRequest request) {
        ErrorResponse response = new ErrorResponse(Instant.now(), exception.getStatus().value(),
                exception.getCode(), exception.getMessage(), request.getRequestURI(), exception.getDetails());
        return ResponseEntity.status(exception.getStatus()).body(response);
    }

    @ExceptionHandler({HttpMessageNotReadableException.class, MissingRequestHeaderException.class,
            MethodArgumentTypeMismatchException.class})
    public ResponseEntity<ErrorResponse> handleBadRequest(Exception exception, HttpServletRequest request) {
        ErrorResponse response = new ErrorResponse(Instant.now(), 400, TransferErrorCode.INVALID_REQUEST,
                exception.getMessage(), request.getRequestURI(), Map.of());
        return ResponseEntity.badRequest().body(response);
    }
}
