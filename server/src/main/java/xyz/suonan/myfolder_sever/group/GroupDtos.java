package xyz.suonan.myfolder_sever.group;

import java.time.Instant;
import java.util.List;

public final class GroupDtos {
    private GroupDtos() {}

    public record CreateGroupRequest(String name) {}
    public record RenameGroupRequest(String name) {}
    public record AddMemberRequest(String account, String permission) {}
    public record UpdateMemberRequest(String permission) {}
    public record Member(String account, String displayName, String permission, Instant joinedAt) {}
    public record Group(String groupId, String name, String ownerAccount, String myPermission,
                        Instant createdAt, List<Member> members) {}
    public record FolderAclRequest(String folderPath, String account, String permission) {}
    public record FolderAcl(String folderPath, String account, String displayName, String permission) {}
}
