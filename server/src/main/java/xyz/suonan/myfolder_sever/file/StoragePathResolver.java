package xyz.suonan.myfolder_sever.file;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

@Component
public class StoragePathResolver {
    private static final String INTERNAL_DIRECTORY = ".myfolder";
    private final Path root;

    public StoragePathResolver(@Value("${basePath}") String basePath) {
        this.root = Path.of(basePath).toAbsolutePath().normalize();
    }

    public Path root() { return root; }

    public Path privateRoot(String account) {
        return isolatedRoot("users", account);
    }

    public Path groupRoot(String groupId) {
        return isolatedRoot("groups", groupId);
    }

    private Path isolatedRoot(String kind, String id) {
        // Account names may come from an OAuth display name and can therefore
        // contain non-ASCII letters. Keep the value to one safe path segment,
        // while explicitly rejecting the two traversal-only segment names.
        if (id == null || id.equals(".") || id.equals("..")
                || !id.matches("[\\p{L}\\p{N}._-]{1,80}")) throw invalidPath(id);
        Path kindRoot = root.resolve(kind).toAbsolutePath().normalize();
        Path scope = kindRoot.resolve(id).toAbsolutePath().normalize();
        if (!scope.startsWith(kindRoot) || scope.equals(kindRoot)) throw invalidPath(id);
        try {
            Files.createDirectories(scope);
        } catch (IOException exception) {
            throw new FileOperationException(HttpStatus.INTERNAL_SERVER_ERROR, "STORAGE_ERROR",
                    "无法创建用户存储空间", exception, details(id));
        }
        return scope;
    }

    public Path resolve(String userPath) {
        return resolve(root, userPath);
    }

    public Path resolve(Path scopeRoot, String userPath) {
        if (userPath == null || userPath.indexOf('\0') >= 0) throw invalidPath(userPath);
        String portable = userPath.replace('\\', '/');
        Path relative;
        try {
            relative = Path.of(portable);
        } catch (RuntimeException exception) {
            throw invalidPath(userPath);
        }
        if (relative.isAbsolute()) throw invalidPath(userPath);
        Path normalizedScope = scopeRoot.toAbsolutePath().normalize();
        Path candidate = normalizedScope.resolve(relative).normalize();
        if (!candidate.startsWith(normalizedScope)) throw invalidPath(userPath);
        if (isInternal(candidate)) throw reservedPath(userPath);
        verifyExistingAncestor(normalizedScope, candidate, userPath);
        return candidate;
    }

    public Path resolveExisting(String userPath) {
        return resolveExisting(root, userPath);
    }

    public Path resolveExisting(Path scopeRoot, String userPath) {
        Path candidate = resolve(scopeRoot, userPath);
        if (!Files.exists(candidate)) {
            throw new FileOperationException(HttpStatus.NOT_FOUND, "FILE_NOT_FOUND",
                    "文件或目录不存在", null, details(userPath));
        }
        verifyRealPath(scopeRoot, candidate, userPath);
        return candidate;
    }

    public String relative(Path path) {
        return root.relativize(path.toAbsolutePath().normalize()).toString().replace('\\', '/');
    }

    public String relative(Path scopeRoot, Path path) {
        return scopeRoot.toAbsolutePath().normalize().relativize(path.toAbsolutePath().normalize())
                .toString().replace('\\', '/');
    }

    public boolean isInternal(Path path) {
        Path internalRoot = root.resolve(INTERNAL_DIRECTORY).normalize();
        return path.toAbsolutePath().normalize().startsWith(internalRoot);
    }

    private void verifyExistingAncestor(Path scopeRoot, Path candidate, String userPath) {
        if (!Files.exists(scopeRoot)) return;
        Path existing = candidate;
        while (existing != null && !Files.exists(existing)) existing = existing.getParent();
        if (existing != null) verifyRealPath(scopeRoot, existing, userPath);
    }

    private void verifyRealPath(Path scopeRoot, Path candidate, String userPath) {
        try {
            Path realRoot = Files.exists(scopeRoot) ? scopeRoot.toRealPath() : scopeRoot;
            Path realCandidate = candidate.toRealPath();
            if (!realCandidate.startsWith(realRoot)) throw invalidPath(userPath);
        } catch (IOException exception) {
            throw new FileOperationException(HttpStatus.INTERNAL_SERVER_ERROR, "STORAGE_ERROR",
                    "无法解析服务器存储路径", exception, details(userPath));
        }
    }

    private FileOperationException invalidPath(String path) {
        return new FileOperationException(HttpStatus.BAD_REQUEST, "INVALID_PATH",
                "路径必须位于服务器文件根目录内", null, details(path));
    }

    private FileOperationException reservedPath(String path) {
        return new FileOperationException(HttpStatus.FORBIDDEN, "RESERVED_PATH",
                ".myfolder 是服务端内部目录，不能通过文件接口访问", null, details(path));
    }

    private java.util.Map<String, Object> details(String path) {
        return java.util.Map.of("path", path == null ? "" : path);
    }
}
