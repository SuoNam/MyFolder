package xyz.suonan.myfolder_sever.group;

import org.springframework.dao.DuplicateKeyException;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import xyz.suonan.myfolder_sever.auth.AuthException;
import xyz.suonan.myfolder_sever.auth.AuthRepository;

import java.util.List;
import java.util.Locale;
import java.util.UUID;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.HexFormat;

@Service
public class GroupService {
    private final GroupRepository groups;
    private final AuthRepository users;

    public GroupService(GroupRepository groups, AuthRepository users) { this.groups = groups; this.users = users; }

    public List<GroupDtos.Group> list(String account) {
        return groups.forUser(requireAccount(account)).stream().map(row -> dto(row, account)).toList();
    }

    @Transactional
    public GroupDtos.Group create(String account, String rawName) {
        account = requireAccount(account);
        String name = validName(rawName);
        String id = UUID.randomUUID().toString();
        groups.create(id, name, account);
        return dto(groups.find(id).orElseThrow(), account);
    }

    public GroupDtos.Group get(String account, String id) {
        requirePermission(id, account, false);
        return dto(requireGroup(id), account);
    }

    public GroupDtos.Group rename(String account, String id, String name) {
        requireOwner(id, account);
        groups.rename(id, validName(name));
        return dto(requireGroup(id), account);
    }

    @Transactional
    public void delete(String account, String id) {
        requireOwner(id, account);
        groups.delete(id);
    }

    public GroupDtos.Group addMember(String account, String id, String memberAccount, String rawPermission) {
        requireOwner(id, account);
        String target = requireAccount(memberAccount);
        if (!users.accountExists(target)) throw error(HttpStatus.NOT_FOUND, "MEMBER_NOT_FOUND", "用户不存在");
        if (requireGroup(id).owner().equals(target)) throw error(HttpStatus.CONFLICT, "OWNER_ALREADY_MEMBER", "群组所有者已经是成员");
        try {
            groups.addMember(id, target, memberPermission(rawPermission));
        } catch (DuplicateKeyException exception) {
            throw error(HttpStatus.CONFLICT, "MEMBER_EXISTS", "该用户已经在群组中");
        }
        return dto(requireGroup(id), account);
    }

    public GroupDtos.Group updateMember(String account, String id, String memberAccount, String permission) {
        requireOwner(id, account);
        if (requireGroup(id).owner().equals(memberAccount)) throw error(HttpStatus.BAD_REQUEST, "OWNER_IMMUTABLE", "不能修改所有者权限");
        groups.updateMember(id, memberAccount, memberPermission(permission));
        return dto(requireGroup(id), account);
    }

    public void removeMember(String account, String id, String memberAccount) {
        requireOwner(id, account);
        if (requireGroup(id).owner().equals(memberAccount)) throw error(HttpStatus.BAD_REQUEST, "OWNER_IMMUTABLE", "不能移除群组所有者");
        groups.removeMember(id, memberAccount);
    }

    public String requirePermission(String id, String account, boolean write) {
        requireGroup(id);
        if (users.isSuperAdmin(account)) return "SUPER_ADMIN";
        String permission = groups.permission(id, requireAccount(account))
                .orElseThrow(() -> error(HttpStatus.FORBIDDEN, "GROUP_ACCESS_DENIED", "你不是该群组成员"));
        if (write && "READ".equals(permission)) throw error(HttpStatus.FORBIDDEN, "GROUP_READ_ONLY", "你在该群组中只有读取权限");
        return permission;
    }

    public String requirePathPermission(String id, String account, String rawPath, boolean write) {
        String base = requirePermission(id, account, false);
        if ("SUPER_ADMIN".equals(base) || "OWNER".equals(base)) return base;
        String path = normalizePath(rawPath);
        String cursor = path;
        while (true) {
            var override = groups.folderPermission(id, account, hash(cursor));
            if (override.isPresent()) {
                String permission = override.get();
                if ("NONE".equals(permission) || (write && "READ".equals(permission))) {
                    throw error(HttpStatus.FORBIDDEN, "FOLDER_ACCESS_DENIED", "你没有该目录所需的权限");
                }
                return permission;
            }
            int slash = cursor.lastIndexOf('/');
            if (slash < 0) cursor = ""; else cursor = cursor.substring(0, slash);
            if (cursor.isEmpty()) {
                var root = groups.folderPermission(id, account, hash(""));
                if (root.isPresent()) {
                    String permission = root.get();
                    if ("NONE".equals(permission) || (write && "READ".equals(permission))) {
                        throw error(HttpStatus.FORBIDDEN, "FOLDER_ACCESS_DENIED", "你没有该目录所需的权限");
                    }
                    return permission;
                }
                break;
            }
        }
        if (write && "READ".equals(base)) {
            throw error(HttpStatus.FORBIDDEN, "GROUP_READ_ONLY", "你在该群组中只有读取权限");
        }
        return base;
    }

    public List<GroupDtos.FolderAcl> folderAcl(String account, String id, String folderPath) {
        requireOwner(id, account);
        String path = normalizePath(folderPath);
        return groups.acl(id, path).stream().map(a -> new GroupDtos.FolderAcl(
                a.folderPath(), a.account(), a.displayName(), a.permission())).toList();
    }

    public List<GroupDtos.FolderAcl> setFolderAcl(String account, String id, GroupDtos.FolderAclRequest request) {
        requireOwner(id, account);
        String path = normalizePath(request.folderPath());
        if (groups.permission(id, request.account()).isEmpty()) throw error(HttpStatus.BAD_REQUEST, "NOT_A_GROUP_MEMBER", "该用户不是群组成员");
        if (requireGroup(id).owner().equals(request.account())) throw error(HttpStatus.BAD_REQUEST, "OWNER_IMMUTABLE", "群主权限不能被目录规则限制");
        String permission = request.permission() == null ? "" : request.permission().trim().toUpperCase(Locale.ROOT);
        if (!List.of("NONE", "READ", "WRITE", "MANAGE").contains(permission)) throw error(HttpStatus.BAD_REQUEST, "INVALID_FOLDER_PERMISSION", "目录权限只能是 NONE、READ、WRITE 或 MANAGE");
        groups.setAcl(id, path, hash(path), request.account(), permission);
        return folderAcl(account, id, path);
    }

    public void removeFolderAcl(String account, String id, String folderPath, String memberAccount) {
        requireOwner(id, account);
        groups.removeAcl(id, hash(normalizePath(folderPath)), memberAccount);
    }

    private void requireOwner(String id, String account) {
        String permission = requirePermission(id, account, true);
        if (!"OWNER".equals(permission) && !"SUPER_ADMIN".equals(permission)) {
            throw error(HttpStatus.FORBIDDEN, "GROUP_OWNER_REQUIRED", "只有群组所有者可以执行此操作");
        }
    }

    private GroupDtos.Group dto(GroupRepository.GroupRow row, String account) {
        String permission = groups.permission(row.id(), account).orElse("NONE");
        List<GroupDtos.Member> members = groups.members(row.id()).stream()
                .map(m -> new GroupDtos.Member(m.account(), m.displayName(), m.permission(), m.createdAt())).toList();
        return new GroupDtos.Group(row.id(), row.name(), row.owner(), permission, row.createdAt(), members);
    }

    private GroupRepository.GroupRow requireGroup(String id) {
        if (id == null || id.isBlank()) throw error(HttpStatus.BAD_REQUEST, "GROUP_ID_REQUIRED", "缺少 groupId");
        return groups.find(id).orElseThrow(() -> error(HttpStatus.NOT_FOUND, "GROUP_NOT_FOUND", "群组不存在"));
    }

    private String validName(String raw) {
        String value = raw == null ? "" : raw.trim();
        if (value.length() < 2 || value.length() > 80 || value.chars().anyMatch(Character::isISOControl)) {
            throw error(HttpStatus.BAD_REQUEST, "INVALID_GROUP_NAME", "群组名称需要 2–80 个字符");
        }
        return value;
    }

    private String memberPermission(String raw) {
        String value = raw == null ? "" : raw.trim().toUpperCase(Locale.ROOT);
        if (!value.equals("READ") && !value.equals("WRITE")) {
            throw error(HttpStatus.BAD_REQUEST, "INVALID_GROUP_PERMISSION", "成员权限只能是 READ 或 WRITE");
        }
        return value;
    }

    private String requireAccount(String account) {
        if (account == null || account.isBlank()) throw error(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", "请先登录");
        return account;
    }

    private String normalizePath(String raw) {
        if (raw == null || raw.isBlank()) return "";
        try { return xyz.suonan.myfolder_sever.Utils.SafeRelativePath.normalize(raw); }
        catch (IllegalArgumentException e) { throw error(HttpStatus.BAD_REQUEST, "INVALID_PATH", "目录路径不合法"); }
    }

    private String hash(String value) {
        try { return HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(value.getBytes(StandardCharsets.UTF_8))); }
        catch (java.security.NoSuchAlgorithmException e) { throw new IllegalStateException(e); }
    }

    private AuthException error(HttpStatus status, String code, String message) { return new AuthException(status, code, message); }
}
