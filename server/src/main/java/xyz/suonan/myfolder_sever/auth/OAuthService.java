package xyz.suonan.myfolder_sever.auth;

import com.fasterxml.jackson.databind.JsonNode;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.http.*;
import org.springframework.http.client.JdkClientHttpRequestFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.web.client.RestClient;
import org.springframework.web.client.RestClientResponseException;
import org.springframework.web.util.UriComponentsBuilder;
import xyz.suonan.myfolder_sever.Config.AuthProperties;

import java.net.URI;
import java.net.InetSocketAddress;
import java.net.ProxySelector;
import java.net.http.HttpClient;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.time.Instant;
import java.time.Duration;
import java.util.*;

@Service
public class OAuthService {
    private static final Logger log = LoggerFactory.getLogger(OAuthService.class);
    private static final String USER_AGENT = "MyFolder/1.1.1 (+https://myfolder.com.cn)";
    private final AuthProperties properties;
    private final AuthRepository repository;
    private final RestClient http;
    private final RestClient googleHttp;
    private final RestClient githubHttp;

    public OAuthService(AuthProperties properties, AuthRepository repository, RestClient.Builder builder) {
        this.properties = properties;
        this.repository = repository;
        HttpClient jdkClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(6))
                .followRedirects(HttpClient.Redirect.NORMAL)
                .build();
        JdkClientHttpRequestFactory requestFactory = new JdkClientHttpRequestFactory(jdkClient);
        requestFactory.setReadTimeout(Duration.ofSeconds(12));
        this.http = builder.requestFactory(requestFactory)
                .defaultHeader(HttpHeaders.USER_AGENT, USER_AGENT)
                .build();
        this.googleHttp = providerHttp(properties.getOauth().getGoogle());
        this.githubHttp = providerHttp(properties.getOauth().getGithub());
    }

    public List<AuthDtos.ProviderStatus> providers() {
        return List.of(status("nyauth"), status("google"), status("github"));
    }

    @Transactional
    public AuthDtos.OAuthStartResponse start(String rawProvider, AuthDtos.OAuthStartRequest request, String account) {
        String provider = provider(rawProvider);
        AuthProperties.Provider config = config(provider);
        if (!config.configured()) throw new AuthException(HttpStatus.SERVICE_UNAVAILABLE, "OAUTH_NOT_CONFIGURED", provider + " 登录尚未配置");
        AuthDtos.ClientType clientType = request.clientType() == null ? AuthDtos.ClientType.WEB : request.clientType();
        String returnUri = validateReturnUri(clientType, request.returnUri());
        boolean link = request.link();
        if (link && (account == null || account.isBlank())) throw unauthorized();
        String state = CryptoTokens.randomUrlToken(32);
        String verifier = CryptoTokens.randomUrlToken(48);
        String challenge = base64Sha256(verifier);
        Instant expiry = Instant.now().plusSeconds(600);
        repository.insertOAuthTransaction(CryptoTokens.sha256(state), provider, clientType.name(),
                link ? "LINK" : "LOGIN", link ? account : null, verifier, returnUri, expiry);

        UriComponentsBuilder uri = UriComponentsBuilder.fromHttpUrl(authorizeEndpoint(provider))
                .queryParam("client_id", config.getClientId())
                .queryParam("redirect_uri", callback(provider))
                .queryParam("response_type", "code")
                .queryParam("state", state)
                .queryParam("code_challenge", challenge)
                .queryParam("code_challenge_method", "S256");
        if (provider.equals("github")) {
            uri.queryParam("scope", "read:user user:email");
        } else {
            uri.queryParam("scope", "openid profile email");
            // OIDC providers use nonce to bind the authorization response to
            // this transaction. Deriving it from two per-request random values
            // keeps it unpredictable without adding another stored column.
            uri.queryParam("nonce", base64Sha256(state + ":" + verifier));
            // Nyauth does not advertise prompt support and rejects this Google-specific hint.
            if (provider.equals("google")) uri.queryParam("prompt", "select_account");
        }
        // Query values such as OAuth scopes contain spaces. Build first and let
        // Spring encode individual components instead of claiming they are
        // already encoded (which rejects valid scope values at runtime).
        return new AuthDtos.OAuthStartResponse(uri.build().encode().toUriString(), expiry);
    }

    @Transactional
    public String callback(String rawProvider, String state, String code) {
        String provider = provider(rawProvider);
        if (state == null || code == null) throw bad("OAUTH_CALLBACK_INVALID", "OAuth 回调参数不完整");
        AuthRepository.OAuthTransactionRow tx = repository.consumeOAuthTransaction(CryptoTokens.sha256(state))
                .orElseThrow(() -> bad("OAUTH_STATE_INVALID", "OAuth 登录请求已失效"));
        if (!provider.equals(tx.provider()) || tx.expiresAt().isBefore(Instant.now())) {
            throw bad("OAUTH_STATE_INVALID", "OAuth 登录请求已失效");
        }
        ProviderUser remote;
        try {
            remote = fetchUser(provider, code, tx.verifier());
        } catch (AuthException e) {
            return appendQuery(tx.returnUri(), Map.of(
                    "oauthError", e.code(),
                    "oauthErrorDescription", e.getMessage(),
                    "provider", provider));
        } catch (Exception e) {
            log.warn("OAuth profile request failed for provider {}: {}", provider, e.toString());
            return appendQuery(tx.returnUri(), Map.of(
                    "oauthError", "OAUTH_PROFILE_FAILED",
                    "oauthErrorDescription", provider + " 用户资料读取失败，请重试",
                    "provider", provider));
        }
        if (remote.subject() == null || remote.subject().isBlank()) throw bad("OAUTH_IDENTITY_INVALID", "登录提供方未返回有效身份");

        String account;
        Optional<AuthRepository.OAuthIdentityRow> existing = repository.identity(provider, remote.subject());
        if (tx.flowType().equals("LINK")) {
            if (existing.isPresent() && !existing.get().account().equals(tx.account())) {
                throw new AuthException(HttpStatus.CONFLICT, "OAUTH_ALREADY_BOUND", "该第三方账号已绑定其他 MyFolder 账号");
            }
            if (existing.isEmpty()) repository.addIdentity(provider, remote.subject(), tx.account(), remote.email(), remote.username());
            return appendQuery(tx.returnUri(), Map.of("linked", provider));
        }

        if (existing.isPresent()) {
            account = existing.get().account();
        } else {
            AuthRepository.UserRow local = remote.emailVerified() && remote.email() != null
                    ? repository.userByEmail(remote.email()).orElse(null) : null;
            if (local == null) {
                account = uniqueAccount(remote.username(), provider);
                repository.createUser(account, null, remote.emailVerified() ? remote.email() : null, remote.emailVerified());
            } else account = local.account();
            try {
                repository.addIdentity(provider, remote.subject(), account, remote.email(), remote.username());
            } catch (DuplicateKeyException e) {
                throw new AuthException(HttpStatus.CONFLICT, "OAUTH_ALREADY_BOUND", "该第三方账号已绑定其他 MyFolder 账号");
            }
        }
        String oneTimeCode = CryptoTokens.randomUrlToken(32);
        repository.insertLoginCode(CryptoTokens.sha256(oneTimeCode), account, tx.clientType(), Instant.now().plusSeconds(120));
        return appendQuery(tx.returnUri(), Map.of("code", oneTimeCode, "provider", provider));
    }

    @Transactional
    public String callbackFailure(String rawProvider, String state, String error, String description) {
        String provider = provider(rawProvider);
        if (state == null || state.isBlank()) {
            throw bad("OAUTH_STATE_INVALID", "OAuth 登录状态无效");
        }
        AuthRepository.OAuthTransactionRow tx = repository.consumeOAuthTransaction(CryptoTokens.sha256(state))
                .orElseThrow(() -> bad("OAUTH_STATE_INVALID", "OAuth 登录请求已失效"));
        if (!provider.equals(tx.provider()) || tx.expiresAt().isBefore(Instant.now())) {
            throw bad("OAUTH_STATE_INVALID", "OAuth 登录请求已失效");
        }
        String safeError = error == null || error.isBlank() ? "oauth_failed" : error;
        String safeDescription = description == null || description.isBlank()
                ? provider + " 拒绝了登录请求" : description;
        return appendQuery(tx.returnUri(), Map.of(
                "oauthError", safeError,
                "oauthErrorDescription", safeDescription,
                "provider", provider));
    }

    public String exchange(String code, AuthDtos.ClientType type) {
        if (code == null || type == null) throw bad("OAUTH_CODE_INVALID", "OAuth 登录凭据无效");
        return repository.consumeLoginCode(CryptoTokens.sha256(code), type.name())
                .orElseThrow(() -> bad("OAUTH_CODE_INVALID", "OAuth 登录凭据无效或已使用"));
    }

    @Transactional
    public void unlink(String account, String rawProvider) {
        String provider = provider(rawProvider);
        AuthRepository.UserRow user = repository.userByAccount(account).orElseThrow(OAuthService::unauthorized);
        if (user.password() == null && repository.identityCount(account) <= 1) {
            throw bad("LAST_LOGIN_METHOD", "请先设置密码或绑定其他登录方式，再解除此账号");
        }
        repository.removeIdentity(account, provider);
    }

    private ProviderUser fetchUser(String provider, String code, String verifier) {
        AuthProperties.Provider cfg = config(provider);
        var form = new LinkedMultiValueMap<String, String>();
        form.add("client_id", cfg.getClientId());
        form.add("client_secret", cfg.getClientSecret());
        form.add("code", code);
        form.add("redirect_uri", callback(provider));
        form.add("grant_type", "authorization_code");
        form.add("code_verifier", verifier);
        JsonNode token;
        try {
            RestClient providerClient = providerHttp(provider);
            token = providerClient.post().uri(tokenEndpoint(provider)).contentType(MediaType.APPLICATION_FORM_URLENCODED)
                    .accept(MediaType.APPLICATION_JSON).body(form).retrieve().body(JsonNode.class);
        } catch (Exception e) {
            if (e instanceof RestClientResponseException response) {
                log.warn("OAuth token exchange failed for provider {} with HTTP {}", provider, response.getStatusCode().value());
            } else {
                log.warn("OAuth token exchange failed for provider {}: {}", provider, e.toString());
            }
            String message = provider.equals("google")
                    ? "服务器暂时无法连接 Google，请稍后重试"
                    : "无法完成第三方登录，请重试";
            throw new AuthException(HttpStatus.BAD_GATEWAY, "OAUTH_TOKEN_EXCHANGE_FAILED", message);
        }
        String access = token == null ? null : token.path("access_token").asText(null);
        if (access == null) throw new AuthException(HttpStatus.BAD_GATEWAY, "OAUTH_TOKEN_EXCHANGE_FAILED", "登录提供方未返回访问令牌");
        return provider.equals("github") ? githubUser(access) : oidcUser(provider, access);
    }

    private ProviderUser oidcUser(String provider, String access) {
        RestClient providerClient = providerHttp(provider);
        JsonNode user = providerClient.get().uri(provider.equals("google") ? "https://openidconnect.googleapis.com/v1/userinfo" : "https://nyauth.isyuah.top/userinfo")
                .header("Authorization", "Bearer " + access).retrieve().body(JsonNode.class);
        return new ProviderUser(text(user, "sub"), normalizedEmail(text(user, "email")),
                user != null && user.path("email_verified").asBoolean(false),
                first(text(user, "preferred_username"), text(user, "name"), provider));
    }

    private ProviderUser githubUser(String access) {
        JsonNode user = githubGet("https://api.github.com/user", access);
        JsonNode emails = githubGet("https://api.github.com/user/emails", access);
        String email = null;
        boolean verified = false;
        if (emails != null && emails.isArray()) {
            for (JsonNode item : emails) {
                if (item.path("verified").asBoolean(false) && (item.path("primary").asBoolean(false) || email == null)) {
                    email = normalizedEmail(text(item, "email")); verified = true;
                    if (item.path("primary").asBoolean(false)) break;
                }
            }
        }
        return new ProviderUser(text(user, "id"), email, verified, first(text(user, "login"), "github"));
    }

    private JsonNode githubGet(String uri, String access) {
        return githubHttp.get().uri(uri).header("Authorization", "Bearer " + access)
                .header(HttpHeaders.USER_AGENT, USER_AGENT)
                .header("X-GitHub-Api-Version", "2022-11-28").accept(MediaType.APPLICATION_JSON)
                .retrieve().body(JsonNode.class);
    }

    private RestClient providerHttp(String provider) {
        return switch (provider) {
            case "google" -> googleHttp;
            case "github" -> githubHttp;
            default -> http;
        };
    }

    private RestClient providerHttp(AuthProperties.Provider provider) {
        if (!provider.proxyConfigured()) return http;
        HttpClient proxiedClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(6))
                .followRedirects(HttpClient.Redirect.NORMAL)
                .proxy(ProxySelector.of(new InetSocketAddress(provider.getProxyHost(), provider.getProxyPort())))
                .build();
        JdkClientHttpRequestFactory requestFactory = new JdkClientHttpRequestFactory(proxiedClient);
        requestFactory.setReadTimeout(Duration.ofSeconds(12));
        return RestClient.builder().requestFactory(requestFactory)
                .defaultHeader(HttpHeaders.USER_AGENT, USER_AGENT).build();
    }

    private String uniqueAccount(String preferred, String provider) {
        String base = first(preferred, provider + "_user").replaceAll("[^A-Za-z0-9_\\-.\\p{L}]", "_");
        if (base.length() > 22) base = base.substring(0, 22);
        if (base.length() < 2) base = provider + "_user";
        String candidate = base;
        for (int i = 0; i < 20 && repository.accountExists(candidate); i++) candidate = base + "_" + (1000 + new java.security.SecureRandom().nextInt(9000));
        if (repository.accountExists(candidate)) candidate = provider + "_" + CryptoTokens.randomUrlToken(8).substring(0, 10);
        return candidate;
    }

    private AuthDtos.ProviderStatus status(String provider) { return new AuthDtos.ProviderStatus(provider, config(provider).configured()); }
    private AuthProperties.Provider config(String provider) {
        return switch (provider) {
            case "google" -> properties.getOauth().getGoogle();
            case "github" -> properties.getOauth().getGithub();
            case "nyauth" -> properties.getOauth().getNyauth();
            default -> throw bad("OAUTH_PROVIDER_INVALID", "不支持的登录提供方");
        };
    }
    private String provider(String value) { return configName(value == null ? "" : value.toLowerCase(Locale.ROOT)); }
    private String configName(String value) {
        if (!Set.of("google", "github", "nyauth").contains(value)) throw bad("OAUTH_PROVIDER_INVALID", "不支持的登录提供方");
        return value;
    }
    private String authorizeEndpoint(String provider) { return switch (provider) {
        case "google" -> "https://accounts.google.com/o/oauth2/v2/auth";
        case "github" -> "https://github.com/login/oauth/authorize";
        default -> "https://nyauth.isyuah.top/authorize";
    }; }
    private String tokenEndpoint(String provider) { return switch (provider) {
        case "google" -> "https://oauth2.googleapis.com/token";
        case "github" -> "https://github.com/login/oauth/access_token";
        default -> "https://nyauth.isyuah.top/token";
    }; }
    private String callback(String provider) { return properties.getPublicBaseUrl() + "/user/oauth/" + provider + "/callback"; }

    private String validateReturnUri(AuthDtos.ClientType type, String raw) {
        String fallback = type == AuthDtos.ClientType.DESKTOP ? "myfolder://oauth/callback" : properties.getWebBaseUrl() + "/oauth/callback";
        String value = raw == null || raw.isBlank() ? fallback : raw;
        URI uri;
        try { uri = URI.create(value); } catch (Exception e) { throw bad("RETURN_URI_INVALID", "OAuth 返回地址无效"); }
        if (type == AuthDtos.ClientType.DESKTOP && "myfolder".equalsIgnoreCase(uri.getScheme()) && "oauth".equalsIgnoreCase(uri.getHost())) return value;
        if (type == AuthDtos.ClientType.WEB && "https".equalsIgnoreCase(uri.getScheme())
                && ("web.myfolder.com.cn".equalsIgnoreCase(uri.getHost()) || "web.test.myfolder.com.cn".equalsIgnoreCase(uri.getHost()))) return value;
        throw bad("RETURN_URI_INVALID", "OAuth 返回地址不在允许列表中");
    }

    private static String appendQuery(String base, Map<String, String> values) {
        UriComponentsBuilder uri = UriComponentsBuilder.fromUriString(base);
        values.forEach(uri::queryParam);
        return uri.build().encode().toUriString();
    }
    private static String base64Sha256(String value) {
        try { return Base64.getUrlEncoder().withoutPadding().encodeToString(MessageDigest.getInstance("SHA-256").digest(value.getBytes(StandardCharsets.US_ASCII))); }
        catch (Exception e) { throw new IllegalStateException(e); }
    }
    private static String text(JsonNode node, String name) { return node == null || node.path(name).isMissingNode() || node.path(name).isNull() ? null : node.path(name).asText(); }
    private static String normalizedEmail(String value) { return value == null || value.isBlank() ? null : value.trim().toLowerCase(Locale.ROOT); }
    private static String first(String... values) { for (String value : values) if (value != null && !value.isBlank()) return value; return "user"; }
    private static AuthException bad(String code, String message) { return new AuthException(HttpStatus.BAD_REQUEST, code, message); }
    private static AuthException unauthorized() { return new AuthException(HttpStatus.UNAUTHORIZED, "UNAUTHORIZED", "请先登录"); }

    private record ProviderUser(String subject, String email, boolean emailVerified, String username) {}
}
