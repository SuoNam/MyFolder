package xyz.suonan.myfolder_sever.Config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

import java.time.Duration;

@Component
@ConfigurationProperties(prefix = "auth")
public class AuthProperties {
    private Duration webRefreshTtl = Duration.ofDays(30);
    private Duration desktopRefreshTtl = Duration.ofDays(3650);
    private Duration verificationCodeTtl = Duration.ofMinutes(10);
    private Duration verificationResendDelay = Duration.ofSeconds(60);
    private String publicBaseUrl = "https://api.myfolder.com.cn";
    private String webBaseUrl = "https://web.myfolder.com.cn";
    private final Resend resend = new Resend();
    private final OAuth oauth = new OAuth();

    public Duration getWebRefreshTtl() { return webRefreshTtl; }
    public void setWebRefreshTtl(Duration value) { webRefreshTtl = value; }
    public Duration getDesktopRefreshTtl() { return desktopRefreshTtl; }
    public void setDesktopRefreshTtl(Duration value) { desktopRefreshTtl = value; }
    public Duration getVerificationCodeTtl() { return verificationCodeTtl; }
    public void setVerificationCodeTtl(Duration value) { verificationCodeTtl = value; }
    public Duration getVerificationResendDelay() { return verificationResendDelay; }
    public void setVerificationResendDelay(Duration value) { verificationResendDelay = value; }
    public String getPublicBaseUrl() { return publicBaseUrl; }
    public void setPublicBaseUrl(String value) { publicBaseUrl = trimSlash(value); }
    public String getWebBaseUrl() { return webBaseUrl; }
    public void setWebBaseUrl(String value) { webBaseUrl = trimSlash(value); }
    public Resend getResend() { return resend; }
    public OAuth getOauth() { return oauth; }

    private static String trimSlash(String value) {
        if (value == null) return "";
        return value.replaceAll("/+$", "");
    }

    public static class Resend {
        private String apiKey = "";
        private String from = "MyFolder <no-reply@myfolder.com.cn>";
        public String getApiKey() { return apiKey; }
        public void setApiKey(String value) { apiKey = value == null ? "" : value.trim(); }
        public String getFrom() { return from; }
        public void setFrom(String value) { from = value; }
    }

    public static class OAuth {
        private final Provider google = new Provider();
        private final Provider github = new Provider();
        private final Provider nyauth = new Provider();
        public Provider getGoogle() { return google; }
        public Provider getGithub() { return github; }
        public Provider getNyauth() { return nyauth; }
    }

    public static class Provider {
        private String clientId = "";
        private String clientSecret = "";
        private String proxyHost = "";
        private int proxyPort;
        public String getClientId() { return clientId; }
        public void setClientId(String value) { clientId = value == null ? "" : value.trim(); }
        public String getClientSecret() { return clientSecret; }
        public void setClientSecret(String value) { clientSecret = value == null ? "" : value.trim(); }
        public String getProxyHost() { return proxyHost; }
        public void setProxyHost(String value) { proxyHost = value == null ? "" : value.trim(); }
        public int getProxyPort() { return proxyPort; }
        public void setProxyPort(int value) { proxyPort = value; }
        public boolean proxyConfigured() { return !proxyHost.isBlank() && proxyPort > 0 && proxyPort <= 65535; }
        public boolean configured() { return !clientId.isBlank() && !clientSecret.isBlank(); }
    }
}
