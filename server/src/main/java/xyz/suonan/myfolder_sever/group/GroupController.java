package xyz.suonan.myfolder_sever.group;

import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.web.bind.annotation.*;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Config.OpenApiConfig;

import java.util.List;

@RestController
@RequestMapping("/api/v1/groups")
@Tag(name = "Shared groups", description = "多人共享读写的群组文件空间")
@SecurityRequirement(name = OpenApiConfig.JWT_SCHEME)
public class GroupController {
    private final GroupService service;
    public GroupController(GroupService service) { this.service = service; }

    @GetMapping public BaseMessage<List<GroupDtos.Group>> list(HttpServletRequest r) { return ok(service.list(user(r))); }
    @PostMapping public BaseMessage<GroupDtos.Group> create(@RequestBody GroupDtos.CreateGroupRequest b, HttpServletRequest r) { return ok(service.create(user(r), b.name())); }
    @GetMapping("/{id}") public BaseMessage<GroupDtos.Group> get(@PathVariable String id, HttpServletRequest r) { return ok(service.get(user(r), id)); }
    @PatchMapping("/{id}") public BaseMessage<GroupDtos.Group> rename(@PathVariable String id, @RequestBody GroupDtos.RenameGroupRequest b, HttpServletRequest r) { return ok(service.rename(user(r), id, b.name())); }
    @DeleteMapping("/{id}") public BaseMessage<Object> delete(@PathVariable String id, HttpServletRequest r) { service.delete(user(r), id); return ok(null); }
    @PostMapping("/{id}/members") public BaseMessage<GroupDtos.Group> add(@PathVariable String id, @RequestBody GroupDtos.AddMemberRequest b, HttpServletRequest r) { return ok(service.addMember(user(r), id, b.email(), b.permission())); }
    @PostMapping("/{id}/leave") public BaseMessage<Object> leave(@PathVariable String id, HttpServletRequest r) { service.leave(user(r), id); return ok(null); }
    @PatchMapping("/{id}/members/{account}") public BaseMessage<GroupDtos.Group> update(@PathVariable String id, @PathVariable String account, @RequestBody GroupDtos.UpdateMemberRequest b, HttpServletRequest r) { return ok(service.updateMember(user(r), id, account, b.permission())); }
    @DeleteMapping("/{id}/members/{account}") public BaseMessage<Object> remove(@PathVariable String id, @PathVariable String account, HttpServletRequest r) { service.removeMember(user(r), id, account); return ok(null); }
    @GetMapping("/{id}/acl") public BaseMessage<List<GroupDtos.FolderAcl>> acl(@PathVariable String id, @RequestParam(defaultValue = "") String folderPath, HttpServletRequest r) { return ok(service.folderAcl(user(r), id, folderPath)); }
    @PutMapping("/{id}/acl") public BaseMessage<List<GroupDtos.FolderAcl>> setAcl(@PathVariable String id, @RequestBody GroupDtos.FolderAclRequest b, HttpServletRequest r) { return ok(service.setFolderAcl(user(r), id, b)); }
    @DeleteMapping("/{id}/acl/{account}") public BaseMessage<Object> removeAcl(@PathVariable String id, @PathVariable String account, @RequestParam(defaultValue = "") String folderPath, HttpServletRequest r) { service.removeFolderAcl(user(r), id, folderPath, account); return ok(null); }

    private String user(HttpServletRequest r) { Object value = r.getAttribute("myfolder.userId"); return value == null ? "" : value.toString(); }
    private <T> BaseMessage<T> ok(T data) { return new BaseMessage<>(200, "操作成功", data); }
}
