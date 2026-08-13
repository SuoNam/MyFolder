package xyz.suonan.myfolder_sever.transfer.error;

import org.springframework.http.HttpStatus;

import java.util.Map;

public class TransferException extends RuntimeException {
    private final TransferErrorCode code;
    private final HttpStatus status;
    private final Map<String, Object> details;

    public TransferException(TransferErrorCode code, HttpStatus status, String message) {
        this(code, status, message, Map.of());
    }

    public TransferException(TransferErrorCode code, HttpStatus status, String message, Map<String, Object> details) {
        super(message);
        this.code = code;
        this.status = status;
        this.details = details;
    }

    public TransferErrorCode getCode() {
        return code;
    }

    public HttpStatus getStatus() {
        return status;
    }

    public Map<String, Object> getDetails() {
        return details;
    }
}
