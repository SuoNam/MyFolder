package xyz.suonan.myfolder_sever.auth;

import java.time.Instant;
import java.util.List;
import java.util.Map;

public final class AuthDtos {
    private AuthDtos() {}

    public enum ClientType { WEB, DESKTOP }
    public enum VerificationPurpose { REGISTER, RESET_PASSWORD, SENSITIVE, BIND_EMAIL }

    public record LoginRequest(String account, String password, ClientType clientType) {}
    public record SignupRequest(String account, String password, String email, String emailCode,
                                ClientType clientType) {}
    public record CodeRequest(String email, VerificationPurpose purpose) {}
    public record PasswordResetRequest(String email, String code, String newPassword) {}
    public record RefreshRequest(String refreshToken, ClientType clientType) {}
    public record LogoutRequest(String refreshToken) {}
    public record SessionResponse(String token, String accessToken, String refreshToken,
                                  long expiresIn, Instant refreshExpiresAt, String account, String email) {}
    public record AccountProfile(String account, String displayName, String email, boolean emailVerified,
                                 List<OAuthBinding> oauthBindings) {}
    public record DisplayNameRequest(String displayName) {}
    public record OAuthBinding(String provider, String username, String email, Instant boundAt) {}
    public record OAuthStartRequest(ClientType clientType, String returnUri, boolean link) {}
    public record OAuthStartResponse(String authorizationUrl, Instant expiresAt) {}
    public record OAuthExchangeRequest(String code, ClientType clientType) {}
    public record ProviderStatus(String provider, boolean configured) {}
    public record SensitiveVerifyRequest(String code, String action) {}
    public record SensitiveTicket(String token, Instant expiresAt) {}
    public record MessageResponse(String code, String message, Map<String, Object> details) {}
}
