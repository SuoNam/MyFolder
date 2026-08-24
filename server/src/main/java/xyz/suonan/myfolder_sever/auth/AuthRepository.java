package xyz.suonan.myfolder_sever.auth;

import org.springframework.dao.DuplicateKeyException;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.core.RowMapper;
import org.springframework.stereotype.Repository;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

@Repository
public class AuthRepository {
    private final JdbcTemplate jdbc;

    public AuthRepository(JdbcTemplate jdbc) { this.jdbc = jdbc; }

    public record UserRow(String account, String displayName, String password, String email,
                          Instant emailVerifiedAt, boolean autoAcceptDeviceTransfers) {}
    public record SessionRow(String id, String account, String clientType, Instant expiresAt, boolean revoked) {}
    public record VerificationRow(String id, String email, String purpose, String hash,
                                  Instant createdAt, Instant expiresAt, int attempts) {}
    public record OAuthIdentityRow(String provider, String subject, String account,
                                   String email, String username, Instant createdAt) {}
    public record OAuthTransactionRow(String provider, String clientType, String flowType,
                                      String account, String verifier, String returnUri, Instant expiresAt) {}
    public record AccessRow(String storageTier, String systemRole) {}

    private static final RowMapper<UserRow> USER = (rs, n) -> new UserRow(
            rs.getString("account"), rs.getString("display_name"), rs.getString("password"), rs.getString("email"),
            instant(rs.getTimestamp("email_verified_at")), rs.getBoolean("auto_accept_device_transfers"));

    public Optional<UserRow> userByAccount(String account) {
        return jdbc.query("SELECT account,display_name,password,email,email_verified_at,auto_accept_device_transfers FROM user WHERE account=?", USER, account)
                .stream().findFirst();
    }

    public Optional<UserRow> userByEmail(String email) {
        return jdbc.query("SELECT account,display_name,password,email,email_verified_at,auto_accept_device_transfers FROM user WHERE email=?", USER, email)
                .stream().findFirst();
    }

    public void createUser(String account, String passwordHash, String email, boolean verified) {
        jdbc.update("INSERT INTO user(account,display_name,password,email,email_verified_at) VALUES(?,?,?,?,?)",
                account, account, passwordHash, email, verified ? Timestamp.from(Instant.now()) : null);
    }

    public boolean accountExists(String account) {
        Integer value = jdbc.queryForObject("SELECT COUNT(*) FROM user WHERE account=?", Integer.class, account);
        return value != null && value > 0;
    }

    public AccessRow access(String account) {
        return jdbc.query("SELECT storage_tier,system_role FROM user WHERE account=?",
                (rs, n) -> new AccessRow(rs.getString(1), rs.getString(2)), account).stream()
                .findFirst().orElse(new AccessRow("FREE", "USER"));
    }

    public boolean isSuperAdmin(String account) { return "SUPER_ADMIN".equals(access(account).systemRole()); }

    public void updateAccess(String account, String tier, String role) {
        jdbc.update("UPDATE user SET storage_tier=?,system_role=? WHERE account=?", tier, role, account);
    }

    public void updatePassword(String account, String hash) {
        jdbc.update("UPDATE user SET password=? WHERE account=?", hash, account);
    }

    public void updateEmail(String account, String email) {
        jdbc.update("UPDATE user SET email=?,email_verified_at=CURRENT_TIMESTAMP(3) WHERE account=?", email, account);
    }

    public void updateDisplayName(String account, String displayName) {
        jdbc.update("UPDATE user SET display_name=? WHERE account=?", displayName, account);
    }

    public void updateTransferPreferences(String account, boolean autoAcceptDeviceTransfers) {
        jdbc.update("UPDATE user SET auto_accept_device_transfers=? WHERE account=?",
                autoAcceptDeviceTransfers, account);
    }

    public void insertSession(String id, String account, String clientType, String refreshHash,
                              Instant expiresAt, String userAgent, String ip) {
        jdbc.update("INSERT INTO auth_session(session_id,user_account,client_type,refresh_token_hash,expires_at,user_agent,ip_address) VALUES(?,?,?,?,?,?,?)",
                id, account, clientType, refreshHash, Timestamp.from(expiresAt), limit(userAgent, 255), limit(ip, 64));
    }

    public Optional<SessionRow> sessionByRefreshHash(String hash) {
        return jdbc.query("SELECT session_id,user_account,client_type,expires_at,revoked_at FROM auth_session WHERE refresh_token_hash=?",
                (rs, n) -> new SessionRow(rs.getString(1), rs.getString(2), rs.getString(3),
                        rs.getTimestamp(4).toInstant(), rs.getTimestamp(5) != null), hash).stream().findFirst();
    }

    public void rotateSession(String id, String hash, Instant expiresAt) {
        jdbc.update("UPDATE auth_session SET refresh_token_hash=?,expires_at=?,last_used_at=CURRENT_TIMESTAMP(3) WHERE session_id=? AND revoked_at IS NULL",
                hash, Timestamp.from(expiresAt), id);
    }

    public void revokeSessionByRefresh(String hash) {
        jdbc.update("UPDATE auth_session SET revoked_at=CURRENT_TIMESTAMP(3) WHERE refresh_token_hash=? AND revoked_at IS NULL", hash);
    }

    public void revokeAllSessions(String account) {
        jdbc.update("UPDATE auth_session SET revoked_at=CURRENT_TIMESTAMP(3) WHERE user_account=? AND revoked_at IS NULL", account);
    }

    public Optional<VerificationRow> latestVerification(String email, String purpose) {
        return jdbc.query("SELECT verification_id,email,purpose,code_hash,created_at,expires_at,attempts FROM email_verification WHERE email=? AND purpose=? AND consumed_at IS NULL ORDER BY created_at DESC LIMIT 1",
                (rs, n) -> new VerificationRow(rs.getString(1), rs.getString(2), rs.getString(3), rs.getString(4),
                        rs.getTimestamp(5).toInstant(), rs.getTimestamp(6).toInstant(), rs.getInt(7)),
                email, purpose).stream().findFirst();
    }

    public void insertVerification(String id, String email, String purpose, String hash, Instant expiresAt) {
        jdbc.update("INSERT INTO email_verification(verification_id,email,purpose,code_hash,expires_at) VALUES(?,?,?,?,?)",
                id, email, purpose, hash, Timestamp.from(expiresAt));
    }

    public void incrementVerificationAttempts(String id) {
        jdbc.update("UPDATE email_verification SET attempts=attempts+1 WHERE verification_id=?", id);
    }

    public void consumeVerification(String id) {
        jdbc.update("UPDATE email_verification SET consumed_at=CURRENT_TIMESTAMP(3) WHERE verification_id=? AND consumed_at IS NULL", id);
    }

    public Optional<OAuthIdentityRow> identity(String provider, String subject) {
        return jdbc.query("SELECT provider,provider_subject,user_account,provider_email,provider_username,created_at FROM oauth_identity WHERE provider=? AND provider_subject=?",
                oauthIdentityMapper(), provider, subject).stream().findFirst();
    }

    public List<OAuthIdentityRow> identitiesFor(String account) {
        return jdbc.query("SELECT provider,provider_subject,user_account,provider_email,provider_username,created_at FROM oauth_identity WHERE user_account=? ORDER BY provider",
                oauthIdentityMapper(), account);
    }

    public void addIdentity(String provider, String subject, String account, String email, String username) {
        try {
            jdbc.update("INSERT INTO oauth_identity(identity_id,user_account,provider,provider_subject,provider_email,provider_username) VALUES(UUID(),?,?,?,?,?)",
                    account, provider, subject, email, username);
        } catch (DuplicateKeyException e) {
            throw e;
        }
    }

    public void removeIdentity(String account, String provider) {
        jdbc.update("DELETE FROM oauth_identity WHERE user_account=? AND provider=?", account, provider);
    }

    public int identityCount(String account) {
        Integer value = jdbc.queryForObject("SELECT COUNT(*) FROM oauth_identity WHERE user_account=?", Integer.class, account);
        return value == null ? 0 : value;
    }

    public void insertOAuthTransaction(String stateHash, String provider, String clientType, String flow,
                                       String account, String verifier, String returnUri, Instant expiresAt) {
        jdbc.update("INSERT INTO oauth_transaction(state_hash,provider,client_type,flow_type,user_account,pkce_verifier,return_uri,expires_at) VALUES(?,?,?,?,?,?,?,?)",
                stateHash, provider, clientType, flow, account, verifier, returnUri, Timestamp.from(expiresAt));
    }

    public Optional<OAuthTransactionRow> consumeOAuthTransaction(String stateHash) {
        List<OAuthTransactionRow> rows = jdbc.query("SELECT provider,client_type,flow_type,user_account,pkce_verifier,return_uri,expires_at FROM oauth_transaction WHERE state_hash=?",
                (rs, n) -> new OAuthTransactionRow(rs.getString(1), rs.getString(2), rs.getString(3), rs.getString(4), rs.getString(5), rs.getString(6), rs.getTimestamp(7).toInstant()), stateHash);
        jdbc.update("DELETE FROM oauth_transaction WHERE state_hash=?", stateHash);
        return rows.stream().findFirst();
    }

    public void insertLoginCode(String hash, String account, String clientType, Instant expiresAt) {
        jdbc.update("INSERT INTO oauth_login_code(code_hash,user_account,client_type,expires_at) VALUES(?,?,?,?)",
                hash, account, clientType, Timestamp.from(expiresAt));
    }

    public Optional<String> consumeLoginCode(String hash, String clientType) {
        List<String> rows = jdbc.query("SELECT user_account FROM oauth_login_code WHERE code_hash=? AND client_type=? AND consumed_at IS NULL AND expires_at>CURRENT_TIMESTAMP(3)",
                (rs, n) -> rs.getString(1), hash, clientType);
        if (!rows.isEmpty()) jdbc.update("UPDATE oauth_login_code SET consumed_at=CURRENT_TIMESTAMP(3) WHERE code_hash=?", hash);
        return rows.stream().findFirst();
    }

    private static RowMapper<OAuthIdentityRow> oauthIdentityMapper() {
        return (rs, n) -> new OAuthIdentityRow(rs.getString(1), rs.getString(2), rs.getString(3),
                rs.getString(4), rs.getString(5), rs.getTimestamp(6).toInstant());
    }

    private static Instant instant(Timestamp value) { return value == null ? null : value.toInstant(); }
    private static String limit(String value, int max) {
        if (value == null) return null;
        return value.length() <= max ? value : value.substring(0, max);
    }
}
