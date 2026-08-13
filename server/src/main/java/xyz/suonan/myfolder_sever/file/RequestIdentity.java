package xyz.suonan.myfolder_sever.file;

import jakarta.servlet.http.HttpServletRequest;
import org.springframework.stereotype.Component;
import xyz.suonan.myfolder_sever.Utils.JwtGen;

@Component
public class RequestIdentity {
    private final JwtGen jwt;
    public RequestIdentity(JwtGen jwt) { this.jwt = jwt; }

    public String optional(HttpServletRequest request) {
        Object filtered = request.getAttribute("myfolder.userId");
        if (filtered != null && !filtered.toString().isBlank()) return filtered.toString();
        String token = request.getHeader("Authorization");
        return token != null && jwt.verifyJwt(token) ? jwt.subject(token) : null;
    }

    public String required(HttpServletRequest request) {
        String account = optional(request);
        if (account == null || account.isBlank()) {
            throw new FileOperationException(org.springframework.http.HttpStatus.UNAUTHORIZED,
                    "UNAUTHORIZED", "请先登录");
        }
        return account;
    }
}
