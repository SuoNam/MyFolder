package xyz.suonan.myfolder_sever.auth;

import org.springframework.dao.DuplicateKeyException;
import org.springframework.http.HttpStatus;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.List;
import java.util.regex.Pattern;

@Service
public class AccountService {
    private static final Pattern ACCOUNT = Pattern.compile("^[A-Za-z0-9_\\-.\\p{L}]{2,30}$");
    private final AuthRepository repository;
    private final PasswordEncoder encoder;
    private final VerificationService verification;
    private final SessionService sessions;

    public AccountService(AuthRepository repository, PasswordEncoder encoder,
                          VerificationService verification, SessionService sessions) {
        this.repository = repository;
        this.encoder = encoder;
        this.verification = verification;
        this.sessions = sessions;
    }

    public AuthRepository.UserRow authenticate(String account, String password) {
        String identifier = account == null ? "" : account.trim();
        AuthRepository.UserRow user = (identifier.contains("@")
                ? repository.userByEmail(VerificationService.normalizeEmail(identifier))
                : repository.userByAccount(normalizeAccount(identifier))).orElseThrow(AccountService::badCredentials);
        if (password == null || user.password() == null) throw badCredentials();
        boolean bcrypt = user.password().startsWith("$2a$") || user.password().startsWith("$2b$") || user.password().startsWith("$2y$");
        boolean valid = bcrypt ? encoder.matches(password, user.password())
                : MessageDigest.isEqual(password.getBytes(StandardCharsets.UTF_8), user.password().getBytes(StandardCharsets.UTF_8));
        if (!valid) throw badCredentials();
        if (!bcrypt) repository.updatePassword(user.account(), encoder.encode(password));
        return user;
    }

    @Transactional
    public AuthRepository.UserRow register(AuthDtos.SignupRequest request) {
        String account = normalizeAccount(request.account());
        validatePassword(request.password());
        String email = VerificationService.normalizeEmail(request.email());
        verification.consume(email, AuthDtos.VerificationPurpose.REGISTER, request.emailCode());
        try {
            repository.createUser(account, encoder.encode(request.password()), email, true);
        } catch (DuplicateKeyException e) {
            throw new AuthException(HttpStatus.CONFLICT, "ACCOUNT_OR_EMAIL_EXISTS", "账号或邮箱已被使用");
        }
        return repository.userByAccount(account).orElseThrow();
    }

    @Transactional
    public void resetPassword(AuthDtos.PasswordResetRequest request) {
        String email = VerificationService.normalizeEmail(request.email());
        validatePassword(request.newPassword());
        verification.consume(email, AuthDtos.VerificationPurpose.RESET_PASSWORD, request.code());
        AuthRepository.UserRow user = repository.userByEmail(email)
                .orElseThrow(() -> new AuthException(HttpStatus.BAD_REQUEST, "VERIFICATION_CODE_INVALID", "验证码错误或已失效"));
        repository.updatePassword(user.account(), encoder.encode(request.newPassword()));
        sessions.revokeAll(user.account());
    }

    public AuthDtos.AccountProfile profile(String account) {
        AuthRepository.UserRow user = repository.userByAccount(account)
                .orElseThrow(() -> new AuthException(HttpStatus.NOT_FOUND, "ACCOUNT_NOT_FOUND", "账号不存在"));
        List<AuthDtos.OAuthBinding> bindings = repository.identitiesFor(account).stream()
                .map(i -> new AuthDtos.OAuthBinding(i.provider(), i.username(), i.email(), i.createdAt())).toList();
        return new AuthDtos.AccountProfile(user.account(), user.displayName(), user.email(), user.emailVerifiedAt() != null, bindings);
    }

    @Transactional
    public AuthDtos.AccountProfile updateDisplayName(String account, String rawDisplayName) {
        String displayName = rawDisplayName == null ? "" : rawDisplayName.trim();
        if (displayName.length() < 2 || displayName.length() > 40 || displayName.chars().anyMatch(Character::isISOControl)) {
            throw new AuthException(HttpStatus.BAD_REQUEST, "INVALID_DISPLAY_NAME", "用户名长度需要为 2–40 个字符，且不能包含控制字符");
        }
        if (repository.userByAccount(account).isEmpty()) {
            throw new AuthException(HttpStatus.NOT_FOUND, "ACCOUNT_NOT_FOUND", "账号不存在");
        }
        repository.updateDisplayName(account, displayName);
        return profile(account);
    }

    public static String normalizeAccount(String value) {
        String account = value == null ? "" : value.trim();
        if (!ACCOUNT.matcher(account).matches()) {
            throw new AuthException(HttpStatus.BAD_REQUEST, "INVALID_ACCOUNT", "账号需为 2–30 个字母、数字、汉字、点、横线或下划线");
        }
        return account;
    }

    public static void validatePassword(String value) {
        if (value == null || value.length() < 8 || value.length() > 128) {
            throw new AuthException(HttpStatus.BAD_REQUEST, "WEAK_PASSWORD", "密码长度需为 8–128 个字符");
        }
    }

    private static AuthException badCredentials() {
        return new AuthException(HttpStatus.UNAUTHORIZED, "INVALID_CREDENTIALS", "账号或密码错误");
    }
}
