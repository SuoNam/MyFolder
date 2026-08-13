package xyz.suonan.myfolder_sever.Utils;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jws;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.security.Keys;
import org.springframework.stereotype.Component;
import xyz.suonan.myfolder_sever.Config.JwtProperties;

import java.nio.charset.StandardCharsets;
import java.security.Key;
import java.time.Duration;
import java.util.Date;

@Component
public class JwtGen {
    private final JwtProperties properties;
    private final Key key;

    public JwtGen(JwtProperties properties) {
        this.properties = properties;
        this.key = Keys.hmacShaKeyFor(properties.getSecret().getBytes(StandardCharsets.UTF_8));
    }

    public String genJwt(String account) { return genJwt(account, null, "LEGACY"); }

    public String genJwt(String account, String sessionId, String clientType) {
        Date now = new Date();
        var builder = Jwts.builder()
                .setSubject(account)
                .setIssuer("MyFolder")
                .setIssuedAt(now)
                .setExpiration(new Date(now.getTime() + properties.getExpiration().toMillis()))
                .claim("role", "user")
                .claim("client", clientType);
        if (sessionId != null) builder.claim("sid", sessionId);
        return builder.signWith(key).compact();
    }

    public Boolean verifyJwt(String token) {
        try { parse(token); return true; } catch (Exception e) { return false; }
    }
    public String subject(String token) {
        try { return parse(token).getBody().getSubject(); } catch (Exception e) { return null; }
    }
    public Duration expiration() { return properties.getExpiration(); }

    public String genSensitiveTicket(String account, String action, Duration ttl) {
        Date now = new Date();
        return Jwts.builder().setSubject(account).setIssuer("MyFolder")
                .setIssuedAt(now).setExpiration(new Date(now.getTime() + ttl.toMillis()))
                .claim("type", "step_up").claim("action", action).signWith(key).compact();
    }

    public boolean verifySensitiveTicket(String token, String account, String action) {
        try {
            Claims claims = parse(token).getBody();
            return account.equals(claims.getSubject()) && "step_up".equals(claims.get("type", String.class))
                    && action.equals(claims.get("action", String.class));
        } catch (Exception e) { return false; }
    }

    private Jws<Claims> parse(String token) {
        return Jwts.parserBuilder().setSigningKey(key).build().parseClaimsJws(token);
    }
}
