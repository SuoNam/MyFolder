package xyz.suonan.myfolder_sever.Controller;

import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.media.Schema;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.core.io.Resource;
import org.springframework.http.ContentDisposition;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Config.OpenApiConfig;
import xyz.suonan.myfolder_sever.MyObject.Item.FileBaseItem;
import xyz.suonan.myfolder_sever.file.FileStorageService;
import xyz.suonan.myfolder_sever.file.RequestIdentity;
import xyz.suonan.myfolder_sever.file.StorageScopeService;
import xyz.suonan.myfolder_sever.file.StorageQuotaService;

import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

@RestController
@RequestMapping("/file")
@Tag(name = "File API", description = "用户私有空间与群组共享空间文件管理")
public class FileHttpController {
    private final FileStorageService storage;
    private final StorageScopeService scopes;
    private final RequestIdentity identity;
    private final StorageQuotaService quota;

    public FileHttpController(FileStorageService storage, StorageScopeService scopes, RequestIdentity identity,
                              StorageQuotaService quota) {
        this.storage = storage;
        this.scopes = scopes;
        this.identity = identity;
        this.quota = quota;
    }

    @PostMapping("/getfilelist")
    @Operation(summary = "浏览目录", description = "已登录默认读取本人空间；公开读取需在 body 中提供 owner。群组空间必须登录。")
    public BaseMessage<List<FileBaseItem>> getFileList(@RequestBody DirectoryPathRequest request,
                                                        HttpServletRequest servletRequest) {
        var scope = readScope(servletRequest, request.owner());
        scopes.authorizePath(scope, identity.optional(servletRequest), request.directoryPath(), false);
        return new BaseMessage<>(200, "获取成功", storage.list(scope.root(), request.directoryPath()));
    }

    @GetMapping("/downloadfile")
    @Operation(summary = "下载单个文件")
    public ResponseEntity<Resource> downloadFile(@RequestParam String filePathS,
                                                  @RequestParam(required = false) String owner,
                                                  HttpServletRequest servletRequest) {
        var scope = readScope(servletRequest, owner);
        scopes.authorizePath(scope, identity.optional(servletRequest), filePathS, false);
        Resource resource = storage.download(scope.root(), filePathS);
        String filename = resource.getFilename() == null ? "download" : resource.getFilename();
        ContentDisposition disposition = ContentDisposition.attachment()
                .filename(filename, StandardCharsets.UTF_8).build();
        return ResponseEntity.ok().contentType(MediaType.APPLICATION_OCTET_STREAM)
                .header(HttpHeaders.CONTENT_DISPOSITION, disposition.toString())
                .header(HttpHeaders.ACCESS_CONTROL_EXPOSE_HEADERS, HttpHeaders.CONTENT_DISPOSITION)
                .body(resource);
    }

    @PostMapping("/createfolder")
    @SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
    public BaseMessage<Object> createFolder(@RequestBody PathRequest request, HttpServletRequest servletRequest) {
        var scope = writeScope(servletRequest);
        scopes.authorizePath(scope, identity.required(servletRequest), request.path(), true);
        storage.createFolder(scope.root(), request.path());
        return new BaseMessage<>(200, "创建成功", null);
    }

    @PostMapping("/move")
    @SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
    public BaseMessage<List<BaseMessage<String>>> move(@RequestBody List<MoveRequest> requests,
                                                        HttpServletRequest servletRequest) {
        var scope = writeScope(servletRequest);
        var root = scope.root();
        List<BaseMessage<String>> results = new ArrayList<>();
        for (MoveRequest request : requests) {
            scopes.authorizePath(scope, identity.required(servletRequest), request.targetPath(), true);
            scopes.authorizePath(scope, identity.required(servletRequest), request.newPath(), true);
            storage.move(root, request.targetPath(), request.newPath());
            if ("GROUP".equals(scope.type())) quota.moveGroupPath(scope.id(), request.targetPath(), request.newPath());
            results.add(new BaseMessage<>(200, "移动成功", request.targetPath()));
        }
        return new BaseMessage<>(200, "移动完毕", results);
    }

    @PostMapping("/delete")
    @SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
    public BaseMessage<List<BaseMessage<String>>> delete(@RequestBody List<DeleteRequest> requests,
                                                          HttpServletRequest servletRequest) {
        var scope = writeScope(servletRequest);
        var root = scope.root();
        List<BaseMessage<String>> results = new ArrayList<>();
        for (DeleteRequest request : requests) {
            scopes.authorizePath(scope, identity.required(servletRequest), request.deletePath(), true);
            storage.delete(root, request.deletePath());
            if ("GROUP".equals(scope.type())) quota.removeGroupPath(scope.id(), request.deletePath());
            results.add(new BaseMessage<>(200, "删除成功", request.deletePath()));
        }
        return new BaseMessage<>(200, "删除完毕", results);
    }

    @PostMapping("/copy")
    @SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
    public BaseMessage<List<BaseMessage<String>>> copy(@RequestBody List<MoveRequest> requests,
                                                        HttpServletRequest servletRequest) {
        var scope = writeScope(servletRequest);
        var root = scope.root();
        List<BaseMessage<String>> results = new ArrayList<>();
        for (MoveRequest request : requests) {
            scopes.authorizePath(scope, identity.required(servletRequest), request.targetPath(), false);
            scopes.authorizePath(scope, identity.required(servletRequest), request.newPath(), true);
            String account = identity.required(servletRequest);
            quota.requireAvailable(account, storage.totalSize(root, request.targetPath()));
            storage.copy(root, request.targetPath(), request.newPath());
            if ("GROUP".equals(scope.type())) quota.recordGroupTree(scope.id(), root, request.newPath(), account);
            results.add(new BaseMessage<>(200, "复制成功", request.targetPath()));
        }
        return new BaseMessage<>(200, "复制完毕", results);
    }

    @PostMapping("/move-to-group")
    @SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
    @Operation(summary = "将私人文件移动到群组", description = "调用者必须拥有源文件写权限以及目标群组目录写权限")
    public BaseMessage<List<BaseMessage<String>>> moveToGroup(@RequestBody MoveToGroupRequest request,
                                                               HttpServletRequest servletRequest) {
        return transferToGroup(request, servletRequest, false);
    }

    @PostMapping("/copy-to-group")
    @SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
    @Operation(summary = "将私人文件复制到群组", description = "调用者必须拥有源文件读权限以及目标群组目录写权限")
    public BaseMessage<List<BaseMessage<String>>> copyToGroup(@RequestBody MoveToGroupRequest request,
                                                               HttpServletRequest servletRequest) {
        return transferToGroup(request, servletRequest, true);
    }

    private BaseMessage<List<BaseMessage<String>>> transferToGroup(MoveToGroupRequest request,
                                                                    HttpServletRequest servletRequest,
                                                                    boolean copy) {
        String account = identity.required(servletRequest);
        var sourceScope = scopes.resolve(account, "PRIVATE", "", null, true);
        var targetScope = scopes.resolve(account, "GROUP", request.groupId(), null, true);
        String targetDirectory = request.targetDirectory() == null ? "" : request.targetDirectory();
        scopes.authorizePath(targetScope, account, targetDirectory, true);
        if (request.sourcePaths() == null || request.sourcePaths().isEmpty()) {
            throw new xyz.suonan.myfolder_sever.file.FileOperationException(
                    org.springframework.http.HttpStatus.BAD_REQUEST, "SOURCE_REQUIRED", "请至少选择一个文件或文件夹");
        }
        List<BaseMessage<String>> results = new ArrayList<>();
        for (String sourcePath : request.sourcePaths()) {
            scopes.authorizePath(sourceScope, account, sourcePath, !copy);
            String normalizedSource = xyz.suonan.myfolder_sever.Utils.SafeRelativePath.normalize(sourcePath);
            if (normalizedSource.isBlank()) {
                throw new xyz.suonan.myfolder_sever.file.FileOperationException(
                        org.springframework.http.HttpStatus.BAD_REQUEST, "SOURCE_REQUIRED", "不能移动私人文件根目录");
            }
            String name = Path.of(normalizedSource).getFileName().toString();
            String targetPath = targetDirectory == null || targetDirectory.isBlank()
                    ? name
                    : xyz.suonan.myfolder_sever.Utils.SafeRelativePath.normalize(targetDirectory) + "/" + name;
            scopes.authorizePath(targetScope, account, targetPath, true);
            if (copy) {
                quota.requireAvailable(account, storage.totalSize(sourceScope.root(), normalizedSource));
                storage.copyAcrossScopes(sourceScope.root(), normalizedSource, targetScope.root(), targetPath);
            } else {
                storage.moveAcrossScopes(sourceScope.root(), normalizedSource, targetScope.root(), targetPath);
            }
            quota.recordGroupTree(targetScope.id(), targetScope.root(), targetPath, account);
            results.add(new BaseMessage<>(200, copy ? "已复制到群组" : "已移动到群组", sourcePath));
        }
        return new BaseMessage<>(200, copy ? "复制完毕" : "移动完毕", results);
    }

    private StorageScopeService.Scope readScope(HttpServletRequest request, String owner) {
        return scopes.resolve(identity.optional(request), request.getHeader("X-Storage-Scope"),
                request.getHeader("X-Storage-Scope-Id"), owner, false);
    }

    private StorageScopeService.Scope writeScope(HttpServletRequest request) {
        return scopes.resolve(identity.required(request), request.getHeader("X-Storage-Scope"),
                request.getHeader("X-Storage-Scope-Id"), null, true);
    }

    public record DirectoryPathRequest(@Schema(example = "inbox") String directoryPath,
                                       @Schema(description = "公开读取私有空间时的账号") String owner) {}
    public record PathRequest(@Schema(example = "inbox/new-folder") String path) {}
    public record MoveRequest(@Schema(example = "inbox/old.txt") String targetPath,
                              @Schema(example = "inbox/new.txt") String newPath) {}
    public record DeleteRequest(@Schema(example = "inbox/old.txt") String deletePath) {}
    public record MoveToGroupRequest(String groupId, String targetDirectory, List<String> sourcePaths) {}
}
