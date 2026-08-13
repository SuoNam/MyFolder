package xyz.suonan.myfolder_sever.file;

import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.auth.AuthRepository;
import xyz.suonan.myfolder_sever.group.GroupService;

import java.nio.file.Path;
import java.util.Locale;
import java.util.Map;

@Service
public class StorageScopeService {
    public record Scope(String type, String id, Path root) {}

    private final StoragePathResolver paths;
    private final GroupService groups;
    private final AuthRepository users;

    public StorageScopeService(StoragePathResolver paths, GroupService groups, AuthRepository users) {
        this.paths = paths;
        this.groups = groups;
        this.users = users;
    }

    public Scope resolve(String authenticatedAccount, String rawType, String scopeId,
                         String publicOwner, boolean write) {
        String type = rawType == null || rawType.isBlank() ? "PRIVATE" : rawType.trim().toUpperCase(Locale.ROOT);
        if ("GROUP".equals(type)) {
            if (authenticatedAccount == null || authenticatedAccount.isBlank()) throw denied("群组空间需要登录");
            // Folder ACL may elevate or restrict the group default, so path-aware
            // authorization happens in authorizePath rather than at scope lookup.
            groups.requirePermission(scopeId, authenticatedAccount, false);
            return new Scope("GROUP", scopeId, paths.groupRoot(scopeId));
        }
        if (!"PRIVATE".equals(type)) {
            throw new FileOperationException(HttpStatus.BAD_REQUEST, "INVALID_SCOPE", "scopeType 只能是 PRIVATE 或 GROUP");
        }
        String requestedOwner = scopeId != null && !scopeId.isBlank() ? scopeId.trim()
                : (publicOwner == null || publicOwner.isBlank() ? null : publicOwner.trim());
        String owner;
        if (write) {
            owner = users.isSuperAdmin(authenticatedAccount) && requestedOwner != null
                    ? requestedOwner : authenticatedAccount;
        } else {
            owner = requestedOwner == null ? authenticatedAccount : requestedOwner;
        }
        if (owner == null || owner.isBlank()) {
            throw new FileOperationException(HttpStatus.BAD_REQUEST, "OWNER_REQUIRED", "公开读取私有空间时必须提供 owner");
        }
        if (!users.accountExists(owner)) {
            throw new FileOperationException(HttpStatus.NOT_FOUND, "OWNER_NOT_FOUND", "用户空间不存在");
        }
        return new Scope("PRIVATE", owner, paths.privateRoot(owner));
    }

    public void authorizePath(Scope scope, String account, String path, boolean write) {
        if ("GROUP".equals(scope.type())) groups.requirePathPermission(scope.id(), account, path, write);
    }

    private FileOperationException denied(String message) {
        return new FileOperationException(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", message, null, Map.of());
    }
}
