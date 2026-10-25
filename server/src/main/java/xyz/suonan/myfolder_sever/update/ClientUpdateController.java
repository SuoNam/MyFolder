package xyz.suonan.myfolder_sever.update;

import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

import java.time.Instant;
import java.util.Map;

@RestController
@RequestMapping("/api/v1/updates/desktop")
@Tag(name = "Desktop updates", description = "公开读取 Windows/Linux 客户端更新清单。")
public class ClientUpdateController {
    private final ClientUpdateService service;

    public ClientUpdateController(ClientUpdateService service) {
        this.service = service;
    }

    @GetMapping
    @Operation(summary = "读取适用于当前桌面客户端的更新清单")
    public ClientUpdateManifest manifest(
            @RequestParam String platform,
            @RequestParam(defaultValue = "x86_64") String arch,
            @RequestParam(defaultValue = "stable") String channel,
            @RequestParam String currentVersion,
            @RequestParam(defaultValue = "1") int protocolVersion,
            @RequestParam(defaultValue = "") String installationId) {
        if (protocolVersion != 1) {
            throw new IllegalArgumentException("Unsupported update protocol version: " + protocolVersion);
        }
        return service.manifest(platform, arch, channel, currentVersion, installationId);
    }

    @ExceptionHandler(IllegalArgumentException.class)
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    public Map<String, Object> invalidRequest(IllegalArgumentException error) {
        return Map.of("timestamp", Instant.now().toString(), "status", 400,
                "code", "INVALID_UPDATE_REQUEST", "message", error.getMessage());
    }
}
