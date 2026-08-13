package xyz.suonan.myfolder_sever.auth;

import org.springframework.http.HttpStatus;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import xyz.suonan.myfolder_sever.Config.AuthProperties;

import java.security.SecureRandom;
import java.time.Instant;
import java.util.Locale;
import java.util.UUID;
import java.util.regex.Pattern;

@Service
public class VerificationService {
    private static final Pattern EMAIL = Pattern.compile("^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$");
    private static final SecureRandom RANDOM = new SecureRandom();
    private final AuthRepository repository;
    private final PasswordEncoder encoder;
    private final ResendEmailService emailService;
    private final AuthProperties properties;

    public VerificationService(AuthRepository repository, PasswordEncoder encoder,
                               ResendEmailService emailService, AuthProperties properties) {
        this.repository = repository;
        this.encoder = encoder;
        this.emailService = emailService;
        this.properties = properties;
    }

    @Transactional
    public Instant send(String rawEmail, AuthDtos.VerificationPurpose purpose) {
        String email = normalizeEmail(rawEmail);
        if (purpose == null) throw bad("VERIFICATION_PURPOSE_REQUIRED", "请选择验证码用途");
        Instant now = Instant.now();
        repository.latestVerification(email, purpose.name()).ifPresent(previous -> {
            if (previous.createdAt().plus(properties.getVerificationResendDelay()).isAfter(now)) {
                throw new AuthException(HttpStatus.TOO_MANY_REQUESTS, "CODE_SEND_TOO_FREQUENT", "验证码发送过于频繁，请稍后再试");
            }
        });
        String code = String.format("%06d", RANDOM.nextInt(1_000_000));
        String id = UUID.randomUUID().toString();
        Instant expiresAt = now.plus(properties.getVerificationCodeTtl());
        emailService.sendVerificationCode(email, code, purpose, id);
        repository.insertVerification(id, email, purpose.name(), encoder.encode(code), expiresAt);
        return expiresAt;
    }

    @Transactional
    public void consume(String rawEmail, AuthDtos.VerificationPurpose purpose, String code) {
        String email = normalizeEmail(rawEmail);
        AuthRepository.VerificationRow row = repository.latestVerification(email, purpose.name())
                .orElseThrow(() -> bad("VERIFICATION_CODE_INVALID", "验证码错误或已失效"));
        if (row.expiresAt().isBefore(Instant.now()) || row.attempts() >= 5) {
            throw bad("VERIFICATION_CODE_EXPIRED", "验证码错误或已失效");
        }
        if (code == null || !encoder.matches(code.trim(), row.hash())) {
            repository.incrementVerificationAttempts(row.id());
            throw bad("VERIFICATION_CODE_INVALID", "验证码错误或已失效");
        }
        repository.consumeVerification(row.id());
    }

    public static String normalizeEmail(String value) {
        String email = value == null ? "" : value.trim().toLowerCase(Locale.ROOT);
        if (email.length() > 320 || !EMAIL.matcher(email).matches()) throw bad("INVALID_EMAIL", "请输入有效的邮箱地址");
        return email;
    }

    private static AuthException bad(String code, String message) {
        return new AuthException(HttpStatus.BAD_REQUEST, code, message);
    }
}
