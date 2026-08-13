package xyz.suonan.myfolder_sever.file;

import org.springframework.http.HttpStatus;

import java.util.Map;

public class FileOperationException extends RuntimeException {
    private final HttpStatus status;
    private final String code;
    private final Map<String, Object> details;

    public FileOperationException(HttpStatus status, String code, String message) {
        this(status, code, message, null, Map.of());
    }

    public FileOperationException(HttpStatus status, String code, String message, Throwable cause,
                                  Map<String, Object> details) {
        super(message, cause);
        this.status = status;
        this.code = code;
        this.details = details == null ? Map.of() : Map.copyOf(details);
    }

    public HttpStatus getStatus() { return status; }
    public String getCode() { return code; }
    public Map<String, Object> getDetails() { return details; }
}
