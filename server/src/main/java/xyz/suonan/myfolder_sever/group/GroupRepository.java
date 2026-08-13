package xyz.suonan.myfolder_sever.group;

import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Repository;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

@Repository
public class GroupRepository {
    private final JdbcTemplate jdbc;

    public GroupRepository(JdbcTemplate jdbc) { this.jdbc = jdbc; }

    public record GroupRow(String id, String name, String owner, Instant createdAt) {}
    public record MemberRow(String account, String displayName, String permission, Instant createdAt) {}
    public record AclRow(String folderPath, String account, String displayName, String permission) {}

    public void create(String id, String name, String owner) {
        jdbc.update("INSERT INTO storage_group(group_id,name,owner_account) VALUES(?,?,?)", id, name, owner);
        jdbc.update("INSERT INTO storage_group_member(group_id,user_account,permission) VALUES(?,?, 'OWNER')", id, owner);
    }

    public Optional<GroupRow> find(String id) {
        return jdbc.query("SELECT group_id,name,owner_account,created_at FROM storage_group WHERE group_id=?",
                (rs, n) -> new GroupRow(rs.getString(1), rs.getString(2), rs.getString(3), instant(rs.getTimestamp(4))), id)
                .stream().findFirst();
    }

    public List<GroupRow> forUser(String account) {
        return jdbc.query("SELECT g.group_id,g.name,g.owner_account,g.created_at FROM storage_group g " +
                        "JOIN storage_group_member m ON m.group_id=g.group_id WHERE m.user_account=? ORDER BY g.name,g.group_id",
                (rs, n) -> new GroupRow(rs.getString(1), rs.getString(2), rs.getString(3), instant(rs.getTimestamp(4))), account);
    }

    public Optional<String> permission(String id, String account) {
        return jdbc.query("SELECT permission FROM storage_group_member WHERE group_id=? AND user_account=?",
                (rs, n) -> rs.getString(1), id, account).stream().findFirst();
    }

    public List<MemberRow> members(String id) {
        return jdbc.query("SELECT m.user_account,u.display_name,m.permission,m.created_at FROM storage_group_member m " +
                        "JOIN user u ON u.account=m.user_account WHERE m.group_id=? " +
                        "ORDER BY FIELD(m.permission,'OWNER','WRITE','READ'),u.display_name,u.account",
                (rs, n) -> new MemberRow(rs.getString(1), rs.getString(2), rs.getString(3), instant(rs.getTimestamp(4))), id);
    }

    public void rename(String id, String name) { jdbc.update("UPDATE storage_group SET name=? WHERE group_id=?", name, id); }
    public void delete(String id) { jdbc.update("DELETE FROM storage_group WHERE group_id=?", id); }
    public void addMember(String id, String account, String permission) {
        jdbc.update("INSERT INTO storage_group_member(group_id,user_account,permission) VALUES(?,?,?)", id, account, permission);
    }
    public void updateMember(String id, String account, String permission) {
        jdbc.update("UPDATE storage_group_member SET permission=? WHERE group_id=? AND user_account=? AND permission<>'OWNER'",
                permission, id, account);
    }
    public void removeMember(String id, String account) {
        jdbc.update("DELETE FROM storage_group_member WHERE group_id=? AND user_account=? AND permission<>'OWNER'", id, account);
    }

    public Optional<String> folderPermission(String id, String account, String pathHash) {
        return jdbc.query("SELECT permission FROM storage_group_folder_acl WHERE group_id=? AND user_account=? AND path_hash=?",
                (rs, n) -> rs.getString(1), id, account, pathHash).stream().findFirst();
    }

    public List<AclRow> acl(String id, String folderPath) {
        return jdbc.query("SELECT a.folder_path,a.user_account,u.display_name,a.permission FROM storage_group_folder_acl a " +
                        "JOIN user u ON u.account=a.user_account WHERE a.group_id=? AND a.folder_path=? ORDER BY u.display_name",
                (rs, n) -> new AclRow(rs.getString(1), rs.getString(2), rs.getString(3), rs.getString(4)), id, folderPath);
    }

    public void setAcl(String id, String folderPath, String pathHash, String account, String permission) {
        jdbc.update("INSERT INTO storage_group_folder_acl(group_id,folder_path,path_hash,user_account,permission) VALUES(?,?,?,?,?) " +
                        "ON DUPLICATE KEY UPDATE folder_path=VALUES(folder_path),permission=VALUES(permission)",
                id, folderPath, pathHash, account, permission);
    }

    public void removeAcl(String id, String pathHash, String account) {
        jdbc.update("DELETE FROM storage_group_folder_acl WHERE group_id=? AND path_hash=? AND user_account=?", id, pathHash, account);
    }

    private static Instant instant(Timestamp value) { return value == null ? null : value.toInstant(); }
}
