package xyz.suonan.myfolder_sever.auth;

import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import xyz.suonan.myfolder_sever.Config.AuthProperties;
import xyz.suonan.myfolder_sever.Utils.JwtGen;

import java.time.Duration;
import java.time.Instant;
import java.util.UUID;

@Service
public class SessionService {
    private final AuthRepository repository;
    private final AuthProperties properties;
    private final JwtGen jwt;

    public SessionService(AuthRepository repository, AuthProperties properties, JwtGen jwt) {
        this.repository = repository;
        this.properties = properties;
        this.jwt = jwt;
    }

    @Transactional
    public AuthDtos.SessionResponse create(String account, String email, AuthDtos.ClientType clientType,
                                           String userAgent, String ip) {
        AuthDtos.ClientType type = clientType == null ? AuthDtos.ClientType.WEB : clientType;
        String sessionId = UUID.randomUUID().toString();
        String refresh = CryptoTokens.randomUrlToken(48);
        Instant refreshExpiry = Instant.now().plus(refreshTtl(type));
        repository.insertSession(sessionId, account, type.name(), CryptoTokens.sha256(refresh), refreshExpiry, userAgent, ip);
        String access = jwt.genJwt(account, sessionId, type.name());
        return response(access, refresh, refreshExpiry, account, email);
    }

    @Transactional
    public AuthDtos.SessionResponse refresh(String rawToken, AuthDtos.ClientType requestedType) {
        if (rawToken == null || rawToken.isBlank()) throw unauthorized();
        AuthRepository.SessionRow session = repository.sessionByRefreshHash(CryptoTokens.sha256(rawToken))
                .orElseThrow(SessionService::unauthorized);
        if (session.revoked() || session.expiresAt().isBefore(Instant.now())) throw unauthorized();
        AuthDtos.ClientType type = AuthDtos.ClientType.valueOf(session.clientType());
        if (requestedType != null && requestedType != type) throw unauthorized();
        String replacement = CryptoTokens.randomUrlToken(48);
        Instant expiry = Instant.now().plus(refreshTtl(type));
        repository.rotateSession(session.id(), CryptoTokens.sha256(replacement), expiry);
        AuthRepository.UserRow user = repository.userByAccount(session.account()).orElseThrow(SessionService::unauthorized);
        return response(jwt.genJwt(session.account(), session.id(), type.name()), replacement, expiry,
                session.account(), user.email());
    }

    public void revoke(String refreshToken) {
        if (refreshToken != null && !refreshToken.isBlank()) repository.revokeSessionByRefresh(CryptoTokens.sha256(refreshToken));
    }
    public void revokeAll(String account) { repository.revokeAllSessions(account); }

    private Duration refreshTtl(AuthDtos.ClientType type) {
        return type == AuthDtos.ClientType.DESKTOP ? properties.getDesktopRefreshTtl() : properties.getWebRefreshTtl();
    }

    private AuthDtos.SessionResponse response(String access, String refresh, Instant expiry, String account, String email) {
        return new AuthDtos.SessionResponse(access, access, refresh, jwt.expiration().toSeconds(), expiry, account, email);
    }
    private static AuthException unauthorized() {
        return new AuthException(HttpStatus.UNAUTHORIZED, "SESSION_INVALID", "登录状态已失效，请重新登录");
    }
}
