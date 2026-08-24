package xyz.suonan.myfolder_sever.group;

import org.junit.jupiter.api.Test;
import xyz.suonan.myfolder_sever.auth.AuthException;
import xyz.suonan.myfolder_sever.auth.AuthRepository;

import java.time.Instant;
import java.util.List;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.Mockito.*;

class GroupServiceTest {
    private final GroupRepository groups = mock(GroupRepository.class);
    private final AuthRepository users = mock(AuthRepository.class);
    private final GroupService service = new GroupService(groups, users);

    @Test
    void ownerAddsMemberByVerifiedEmailInsteadOfDisplayName() {
        var row = new GroupRepository.GroupRow("g1", "Team", "owner", Instant.now());
        when(groups.find("g1")).thenReturn(Optional.of(row));
        when(groups.permission("g1", "owner")).thenReturn(Optional.of("OWNER"));
        when(users.userByEmail("member@example.com")).thenReturn(Optional.of(
                new AuthRepository.UserRow("immutable-account", "可重复昵称", "", "member@example.com", Instant.now(), false)));
        when(groups.members("g1")).thenReturn(List.of());

        service.addMember("owner", "g1", " MEMBER@EXAMPLE.COM ", "WRITE");

        verify(groups).addMember("g1", "immutable-account", "WRITE");
    }

    @Test
    void memberCanLeaveAndTheirFolderRulesAreRemoved() {
        var row = new GroupRepository.GroupRow("g1", "Team", "owner", Instant.now());
        when(groups.find("g1")).thenReturn(Optional.of(row));
        when(groups.permission("g1", "member")).thenReturn(Optional.of("WRITE"));

        service.leave("member", "g1");

        verify(groups).removeMemberAcl("g1", "member");
        verify(groups).removeMember("g1", "member");
    }

    @Test
    void ownerMustTransferOrDeleteInsteadOfLeaving() {
        var row = new GroupRepository.GroupRow("g1", "Team", "owner", Instant.now());
        when(groups.find("g1")).thenReturn(Optional.of(row));
        when(groups.permission("g1", "owner")).thenReturn(Optional.of("OWNER"));

        AuthException error = assertThrows(AuthException.class, () -> service.leave("owner", "g1"));
        assertEquals("OWNER_CANNOT_LEAVE", error.code());
        verify(groups, never()).removeMember(anyString(), anyString());
    }
}
