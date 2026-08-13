package xyz.suonan.myfolder_sever.Controller;

import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import xyz.suonan.myfolder_sever.auth.*;

import java.time.Instant;
import java.time.Duration;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.Map;

@RestController
@RequestMapping("/user")
@Tag(name = "Authentication", description = "邮箱验证码、账号登录和可撤销的长期会话。Authorization 仍直接传裸 JWT。")
public class UserController {
    private final AccountService accounts;
    private final SessionService sessions;
    private final VerificationService verification;
    private final AuthRepository repository;
    private final JwtGen jwt;
    private final OAuthService oauth;

    public UserController(AccountService accounts, SessionService sessions, VerificationService verification,
                          AuthRepository repository, JwtGen jwt, OAuthService oauth) {
        this.accounts = accounts;
        this.sessions = sessions;
        this.verification = verification;
        this.repository = repository;
        this.jwt = jwt;
        this.oauth = oauth;
    }

    @PostMapping("/email/code")
    @Operation(summary = "发送邮箱验证码")
    public BaseMessage<Map<String, Object>> sendCode(@RequestBody AuthDtos.CodeRequest request,
                                                      @RequestHeader(value = "Authorization", required = false) String token) {
        String email = VerificationService.normalizeEmail(request.email());
        if (request.purpose() == AuthDtos.VerificationPurpose.REGISTER && repository.userByEmail(email).isPresent()) {
            throw new AuthException(HttpStatus.CONFLICT, "EMAIL_EXISTS", "该邮箱已被使用");
        }
        if (request.purpose() == AuthDtos.VerificationPurpose.RESET_PASSWORD && repository.userByEmail(email).isEmpty()) {
            return ok("如果该邮箱已注册，验证码将发送到邮箱", Map.of("expiresAt", Instant.now().plusSeconds(600)));
        }
        if (request.purpose() == AuthDtos.VerificationPurpose.SENSITIVE) {
            String account = token == null ? null : jwt.subject(token);
            AuthRepository.UserRow user = account == null ? null : repository.userByAccount(account).orElse(null);
            if (user == null || user.email() == null || !user.email().equals(email)) {
                throw new AuthException(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", "请先登录并使用当前账号邮箱验证");
            }
        }
        // Password reset deliberately does not disclose whether an email exists.
        Instant expiresAt = verification.send(email, request.purpose());
        return ok("验证码已发送", Map.of("expiresAt", expiresAt));
    }

    @PostMapping("/signup")
    @Operation(summary = "使用邮箱验证码注册")
    public ResponseEntity<BaseMessage<AuthDtos.SessionResponse>> signUp(
            @RequestBody AuthDtos.SignupRequest request, HttpServletRequest http) {
        AuthRepository.UserRow user = accounts.register(request);
        return ResponseEntity.status(HttpStatus.CREATED).body(ok("注册成功",
                sessions.create(user.account(), user.email(), request.clientType(), http.getHeader("User-Agent"), http.getRemoteAddr())));
    }

    @PostMapping("/login")
    @Operation(summary = "账号或邮箱密码登录")
    public BaseMessage<AuthDtos.SessionResponse> logIn(
            @RequestBody AuthDtos.LoginRequest request, HttpServletRequest http) {
        AuthRepository.UserRow user = accounts.authenticate(request.account(), request.password());
        return ok("登录成功", sessions.create(user.account(), user.email(), request.clientType(),
                http.getHeader("User-Agent"), http.getRemoteAddr()));
    }

    @PostMapping("/refresh")
    @Operation(summary = "轮换刷新令牌并签发新访问令牌")
    public BaseMessage<AuthDtos.SessionResponse> refresh(@RequestBody AuthDtos.RefreshRequest request) {
        return ok("会话已刷新", sessions.refresh(request.refreshToken(), request.clientType()));
    }

    @PostMapping("/logout")
    @Operation(summary = "撤销当前长期会话")
    public BaseMessage<Void> logout(@RequestBody(required = false) AuthDtos.LogoutRequest request) {
        if (request != null) sessions.revoke(request.refreshToken());
        return ok("已退出登录", null);
    }

    @PostMapping("/password/reset")
    @Operation(summary = "使用邮箱验证码重置密码并撤销全部会话")
    public BaseMessage<Void> resetPassword(@RequestBody AuthDtos.PasswordResetRequest request) {
        accounts.resetPassword(request);
        return ok("密码已重置，请重新登录", null);
    }

    @GetMapping("/me")
    @Operation(summary = "当前账号与 OAuth 绑定信息")
    public BaseMessage<AuthDtos.AccountProfile> me(@RequestHeader("Authorization") String token) {
        String account = jwt.subject(token);
        if (account == null) throw new AuthException(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", "登录状态已失效");
        return ok("查询成功", accounts.profile(account));
    }

    @PatchMapping("/me/display-name")
    @Operation(summary = "修改当前账号的显示名称")
    public BaseMessage<AuthDtos.AccountProfile> updateDisplayName(
            @RequestBody AuthDtos.DisplayNameRequest request,
            @RequestHeader("Authorization") String token) {
        String account = jwt.subject(token);
        if (account == null) throw new AuthException(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", "登录状态已失效");
        return ok("用户名已更新", accounts.updateDisplayName(account, request.displayName()));
    }

    @PostMapping("/sensitive/verify")
    @Operation(summary = "校验邮箱验证码并签发 10 分钟敏感操作凭据")
    public BaseMessage<AuthDtos.SensitiveTicket> verifySensitive(
            @RequestBody AuthDtos.SensitiveVerifyRequest request,
            @RequestHeader("Authorization") String token) {
        String account = jwt.subject(token);
        AuthRepository.UserRow user = account == null ? null : repository.userByAccount(account).orElse(null);
        if (user == null || user.email() == null) throw new AuthException(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", "请先登录并绑定邮箱");
        if (request.action() == null || !request.action().matches("oauth:link:(nyauth|google|github)")) {
            throw new AuthException(HttpStatus.BAD_REQUEST, "SENSITIVE_ACTION_INVALID", "不支持的敏感操作");
        }
        verification.consume(user.email(), AuthDtos.VerificationPurpose.SENSITIVE, request.code());
        Instant expiresAt = Instant.now().plus(Duration.ofMinutes(10));
        return ok("验证成功", new AuthDtos.SensitiveTicket(
                jwt.genSensitiveTicket(account, request.action(), Duration.ofMinutes(10)), expiresAt));
    }

    @GetMapping("/oauth/providers")
    @Operation(summary = "查询 OAuth 登录提供方配置状态")
    public BaseMessage<Object> oauthProviders() { return ok("查询成功", oauth.providers()); }

    @PostMapping("/oauth/{provider}/start")
    @Operation(summary = "开始 OAuth 登录或绑定")
    public BaseMessage<AuthDtos.OAuthStartResponse> oauthStart(
            @PathVariable String provider, @RequestBody AuthDtos.OAuthStartRequest request,
            @RequestHeader(value = "Authorization", required = false) String token,
            @RequestHeader(value = "X-Verification-Token", required = false) String verificationToken) {
        String account = token == null ? null : jwt.subject(token);
        if (request.link() && (verificationToken == null
                || !jwt.verifySensitiveTicket(verificationToken, account, "oauth:link:" + provider.toLowerCase()))) {
            throw new AuthException(HttpStatus.FORBIDDEN, "STEP_UP_REQUIRED", "请先使用邮箱验证码确认本次绑定");
        }
        return ok("请在浏览器中继续", oauth.start(provider, request, account));
    }

    @GetMapping("/oauth/{provider}/callback")
    @Operation(summary = "OAuth 提供方回调")
    public ResponseEntity<?> oauthCallback(@PathVariable String provider,
                                              @RequestParam(required = false) String state,
                                              @RequestParam(required = false) String code,
                                              @RequestParam(required = false) String error,
                                              @RequestParam(name = "error_description", required = false) String errorDescription) {
        String destination = error != null || code == null || code.isBlank()
                ? oauth.callbackFailure(provider, state, error, errorDescription)
                : oauth.callback(provider, state, code);
        if (destination.regionMatches(true, 0, "myfolder://", 0, "myfolder://".length())) {
            String encodedDestination = Base64.getEncoder().encodeToString(destination.getBytes(StandardCharsets.UTF_8));
            String providerName = switch (provider.toLowerCase()) {
                case "github" -> "GitHub";
                case "nyauth" -> "Nyauth";
                default -> "Google";
            };
            String html = """
                    <!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
                    <meta name="viewport" content="width=device-width,initial-scale=1">
                    <title>MyFolder 登录完成</title><style>
                    body{margin:0;background:#f2f6f3;color:#10251d;font-family:system-ui,-apple-system,"Segoe UI",sans-serif;display:grid;place-items:center;min-height:100vh}
                    main{width:min(440px,calc(100vw - 40px));background:#fff;border:1px solid #cfddd6;border-radius:16px;padding:36px;box-shadow:0 18px 55px #193e2c18;text-align:center}
                    .icon{width:52px;height:52px;margin:auto;border-radius:14px;background:#20a477;color:white;display:grid;place-items:center;font-size:27px}
                    h1{font-size:23px;margin:18px 0 8px}p{color:#63786f;line-height:1.7;margin:0}a,button{display:inline-block;margin-top:22px;border:0;border-radius:9px;background:#20a477;color:white;padding:11px 22px;font-weight:650;cursor:pointer;text-decoration:none;font-size:14px}
                    button{margin-left:8px;background:#edf3f0;color:#385248}
                    </style></head><body><main><div class="icon">✓</div><h1>已完成 %s 登录</h1>
                    <p id="message">正在返回 MyFolder 客户端……</p><a id="openClient" href="#">返回 MyFolder 客户端</a><button onclick="window.close()">关闭此页</button></main>
                    <script>const target=atob('%s'),openClient=document.getElementById('openClient');openClient.href=target;
                    location.replace(target);setTimeout(()=>openClient.click(),250);
                    setTimeout(()=>{document.getElementById('message').textContent='如果客户端没有自动打开，请点击下方“返回 MyFolder 客户端”。'},1000);
                    document.addEventListener('visibilitychange',()=>{if(document.hidden)setTimeout(()=>window.close(),700)});</script>
                    </body></html>
                    """.formatted(providerName, encodedDestination);
            return ResponseEntity.ok().contentType(MediaType.TEXT_HTML).body(html);
        }
        return ResponseEntity.status(HttpStatus.FOUND).location(java.net.URI.create(destination)).build();
    }

    @PostMapping("/oauth/exchange")
    @Operation(summary = "用一次性 OAuth 登录码创建 MyFolder 会话")
    public BaseMessage<AuthDtos.SessionResponse> oauthExchange(@RequestBody AuthDtos.OAuthExchangeRequest request,
                                                                HttpServletRequest http) {
        String account = oauth.exchange(request.code(), request.clientType());
        AuthRepository.UserRow user = repository.userByAccount(account)
                .orElseThrow(() -> new AuthException(HttpStatus.UNAUTHORIZED, "OAUTH_CODE_INVALID", "OAuth 登录凭据无效"));
        return ok("登录成功", sessions.create(account, user.email(), request.clientType(),
                http.getHeader("User-Agent"), http.getRemoteAddr()));
    }

    @DeleteMapping("/oauth/{provider}")
    @Operation(summary = "解除 OAuth 账号绑定")
    public BaseMessage<Void> oauthUnlink(@PathVariable String provider,
                                         @RequestHeader("Authorization") String token) {
        String account = jwt.subject(token);
        if (account == null) throw new AuthException(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", "请先登录");
        oauth.unlink(account, provider);
        return ok("已解除绑定", null);
    }

    private static <T> BaseMessage<T> ok(String message, T data) { return new BaseMessage<>(200, message, data); }
}
