package xyz.suonan.myfolder_sever.Controller;

import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.mvc.method.annotation.StreamingResponseBody;
import xyz.suonan.myfolder_sever.Utils.FileZipService;
import xyz.suonan.myfolder_sever.file.FileOperationException;
import xyz.suonan.myfolder_sever.file.RequestIdentity;
import xyz.suonan.myfolder_sever.file.StoragePathResolver;
import xyz.suonan.myfolder_sever.file.StorageScopeService;

import java.io.InputStream;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

@RequestMapping("/directory")
@RestController
@Tag(name = "Directory download", description = "目录 ZIP 下载；旧版目录上传协议已停用")
public class DirectoryHttpController {
    private final FileZipService zip;
    private final StoragePathResolver paths;
    private final StorageScopeService scopes;
    private final RequestIdentity identity;

    public DirectoryHttpController(FileZipService zip, StoragePathResolver paths,
                                   StorageScopeService scopes, RequestIdentity identity) {
        this.zip = zip;
        this.paths = paths;
        this.scopes = scopes;
        this.identity = identity;
    }

    @GetMapping("/downloaddirectory")
    @Operation(summary = "下载目录 ZIP")
    public ResponseEntity<StreamingResponseBody> download(@RequestParam String directoryPathS,
                                                           @RequestParam(required = false) String owner,
                                                           HttpServletRequest request) throws java.io.IOException {
        var scope = scopes.resolve(identity.optional(request), request.getHeader("X-Storage-Scope"),
                request.getHeader("X-Storage-Scope-Id"), owner, false);
        scopes.authorizePath(scope, identity.optional(request), directoryPathS, false);
        Path source = paths.resolveExisting(scope.root(), directoryPathS);
        if (!Files.isDirectory(source)) {
            throw new FileOperationException(org.springframework.http.HttpStatus.BAD_REQUEST,
                    "NOT_A_DIRECTORY", "请求路径不是目录");
        }
        Path temporary = Files.createTempFile("myfolder-directory-", ".zip");
        zip.zipFolder(source, temporary);
        String name = source.getFileName() == null ? "files.zip" : source.getFileName() + ".zip";
        StreamingResponseBody stream = output -> {
            try (InputStream input = Files.newInputStream(temporary)) { input.transferTo(output); }
            finally { Files.deleteIfExists(temporary); }
        };
        return ResponseEntity.ok().contentType(MediaType.APPLICATION_OCTET_STREAM)
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" +
                        URLEncoder.encode(name, StandardCharsets.UTF_8) + "\"")
                .header(HttpHeaders.ACCESS_CONTROL_EXPOSE_HEADERS, HttpHeaders.CONTENT_DISPOSITION)
                .body(stream);
    }
}
