package xyz.suonan.myfolder_sever.file;

import org.springframework.http.HttpStatus;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.auth.AuthRepository;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.util.HexFormat;

@Service
public class StorageQuotaService {
    public static final long FREE_BYTES = 5L * 1024 * 1024 * 1024;
    public static final long PREMIUM_BYTES = 20L * 1024 * 1024 * 1024;
    public record Usage(String tier, String systemRole, long usedBytes, Long limitBytes, long availableBytes) {}

    private final AuthRepository users;
    private final StoragePathResolver paths;
    private final JdbcTemplate jdbc;

    public StorageQuotaService(AuthRepository users, StoragePathResolver paths, JdbcTemplate jdbc) {
        this.users = users; this.paths = paths; this.jdbc = jdbc;
    }

    public Usage usage(String account) {
        var access = users.access(account);
        long used = privateUsage(account) + groupUsage(account);
        long reserved = reservedUsage(account);
        if ("SUPER_ADMIN".equals(access.systemRole())) return new Usage(access.storageTier(), access.systemRole(), used, null, Long.MAX_VALUE);
        long limit = "PREMIUM".equals(access.storageTier()) ? PREMIUM_BYTES : FREE_BYTES;
        return new Usage(access.storageTier(), access.systemRole(), used, limit, Math.max(0, limit - used - reserved));
    }

    public synchronized void requireAvailable(String account, long additionalBytes) {
        if (additionalBytes <= 0) return;
        Usage usage = usage(account);
        if (usage.limitBytes() != null && additionalBytes > usage.availableBytes()) {
            throw new FileOperationException(HttpStatus.INSUFFICIENT_STORAGE, "STORAGE_QUOTA_EXCEEDED",
                    "存储空间不足", null, java.util.Map.of("usedBytes", usage.usedBytes(),
                    "limitBytes", usage.limitBytes(), "requiredBytes", additionalBytes));
        }
    }

    public synchronized void reserve(String uploadId, String account, long bytes) {
        requireAvailable(account, bytes);
        jdbc.update("INSERT INTO storage_quota_reservation(upload_id,user_account,reserved_bytes,expires_at) VALUES(?,?,?,DATE_ADD(CURRENT_TIMESTAMP(3),INTERVAL 7 DAY)) " +
                "ON DUPLICATE KEY UPDATE reserved_bytes=VALUES(reserved_bytes),expires_at=VALUES(expires_at)", uploadId, account, bytes);
    }

    public void consume(String uploadId, long bytes) {
        jdbc.update("UPDATE storage_quota_reservation SET reserved_bytes=GREATEST(0,reserved_bytes-?),expires_at=DATE_ADD(CURRENT_TIMESTAMP(3),INTERVAL 7 DAY) WHERE upload_id=?", bytes, uploadId);
    }

    public void release(String uploadId) { jdbc.update("DELETE FROM storage_quota_reservation WHERE upload_id=?", uploadId); }

    public void recordGroupFile(String groupId, String objectPath, String owner, long size) {
        String normalized = normalized(objectPath);
        jdbc.update("INSERT INTO storage_object(scope_type,scope_id,object_path,path_hash,owner_account,size_bytes) VALUES('GROUP',?,?,?,?,?) " +
                        "ON DUPLICATE KEY UPDATE object_path=VALUES(object_path),owner_account=VALUES(owner_account),size_bytes=VALUES(size_bytes)",
                groupId, normalized, hash(normalized), owner, size);
    }

    public void removeGroupPath(String groupId, String objectPath) {
        String normalized = normalized(objectPath);
        String prefix = normalized.isEmpty() ? "%" : normalized + "/%";
        jdbc.update("DELETE FROM storage_object WHERE scope_type='GROUP' AND scope_id=? AND (object_path=? OR object_path LIKE ?)",
                groupId, normalized, prefix);
    }

    public void moveGroupPath(String groupId, String from, String to) {
        String source = normalized(from), target = normalized(to);
        var rows = jdbc.queryForList("SELECT object_path,owner_account,size_bytes FROM storage_object WHERE scope_type='GROUP' AND scope_id=? AND (object_path=? OR object_path LIKE ?)",
                groupId, source, source + "/%");
        removeGroupPath(groupId, source);
        for (var row : rows) {
            String old = row.get("object_path").toString();
            String next = target + old.substring(source.length());
            recordGroupFile(groupId, next, row.get("owner_account").toString(), ((Number) row.get("size_bytes")).longValue());
        }
    }

    public void recordGroupTree(String groupId, Path scopeRoot, String objectPath, String owner) {
        String normalized = normalized(objectPath);
        Path start = scopeRoot.resolve(normalized).normalize();
        try (var walk = Files.walk(start)) {
            walk.filter(Files::isRegularFile).forEach(file -> {
                try { recordGroupFile(groupId, scopeRoot.relativize(file).toString().replace('\\','/'), owner, Files.size(file)); }
                catch (IOException e) { throw new java.io.UncheckedIOException(e); }
            });
        } catch (IOException | java.io.UncheckedIOException e) {
            throw new FileOperationException(HttpStatus.INTERNAL_SERVER_ERROR,"STORAGE_ERROR","无法记录群组文件用量",e,java.util.Map.of());
        }
    }

    private long privateUsage(String account) {
        Path root = paths.privateRoot(account);
        try (var walk = Files.walk(root)) {
            return walk.filter(Files::isRegularFile).mapToLong(p -> { try { return Files.size(p); } catch (IOException e) { return 0; } }).sum();
        } catch (IOException e) { throw new FileOperationException(HttpStatus.INTERNAL_SERVER_ERROR, "STORAGE_ERROR", "无法统计存储用量", e, java.util.Map.of()); }
    }
    private long groupUsage(String account) {
        Long value = jdbc.queryForObject("SELECT COALESCE(SUM(size_bytes),0) FROM storage_object WHERE scope_type='GROUP' AND owner_account=?", Long.class, account);
        return value == null ? 0 : value;
    }
    private long reservedUsage(String account) {
        Long value = jdbc.queryForObject("SELECT COALESCE(SUM(reserved_bytes),0) FROM storage_quota_reservation WHERE user_account=? AND expires_at>CURRENT_TIMESTAMP(3)", Long.class, account);
        return value == null ? 0 : value;
    }
    private String normalized(String value) { return value == null ? "" : value.replace('\\','/').replaceAll("^/+|/+$", ""); }
    private String hash(String value) {
        try { return HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(value.getBytes(StandardCharsets.UTF_8))); }
        catch (Exception e) { throw new IllegalStateException(e); }
    }
}
