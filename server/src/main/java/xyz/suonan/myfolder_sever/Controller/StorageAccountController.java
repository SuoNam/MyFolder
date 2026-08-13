package xyz.suonan.myfolder_sever.Controller;

import jakarta.servlet.http.HttpServletRequest;
import org.springframework.web.bind.annotation.*;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.auth.AuthException;
import xyz.suonan.myfolder_sever.auth.AuthRepository;
import xyz.suonan.myfolder_sever.file.RequestIdentity;
import xyz.suonan.myfolder_sever.file.StorageQuotaService;

@RestController
public class StorageAccountController {
    private final StorageQuotaService quota; private final RequestIdentity identity; private final AuthRepository users;
    public StorageAccountController(StorageQuotaService quota, RequestIdentity identity, AuthRepository users) { this.quota=quota; this.identity=identity; this.users=users; }

    @GetMapping("/user/me/storage") public BaseMessage<StorageQuotaService.Usage> usage(HttpServletRequest r) { return new BaseMessage<>(200,"获取成功",quota.usage(identity.required(r))); }
    @PatchMapping("/api/v1/admin/users/{account}/access") public BaseMessage<Object> access(@PathVariable String account, @RequestBody AccessRequest b, HttpServletRequest r) {
        if (!users.isSuperAdmin(identity.required(r))) throw new AuthException(org.springframework.http.HttpStatus.FORBIDDEN,"SUPER_ADMIN_REQUIRED","需要超级管理员权限");
        String tier = b.storageTier()==null?"FREE":b.storageTier().toUpperCase();
        String role = b.systemRole()==null?"USER":b.systemRole().toUpperCase();
        if (!java.util.List.of("FREE","PREMIUM").contains(tier) || !java.util.List.of("USER","SUPER_ADMIN").contains(role)) throw new AuthException(org.springframework.http.HttpStatus.BAD_REQUEST,"INVALID_ACCESS","无效的用户等级或系统角色");
        users.updateAccess(account,tier,role); return new BaseMessage<>(200,"更新成功",null);
    }
    public record AccessRequest(String storageTier,String systemRole) {}
}
