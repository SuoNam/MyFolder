package xyz.suonan.myfolder_sever.update;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Component
@ConfigurationProperties(prefix = "updates")
public class ClientUpdateProperties {
    private String stableVersion = "1.1.1";
    private String betaVersion = "1.1.2";
    private String minimumSupportedVersion = "1.1.1";
    private String publishedAt = "";
    private String stableReleaseNotes = "";
    private String betaReleaseNotes = "";
    private boolean stableEnabled = true;
    private boolean betaEnabled = true;
    private int stableRolloutPercentage = 100;
    private int betaRolloutPercentage = 100;
    private String signatureAlgorithm = "";
    private String publicKeyVersion = "";
    private String windowsX64UrlTemplate = "";
    private long windowsX64Size;
    private String windowsX64Sha256 = "";
    private String windowsX64SignatureUrl = "";
    private String linuxX64UrlTemplate = "";
    private long linuxX64Size;
    private String linuxX64Sha256 = "";
    private String linuxX64SignatureUrl = "";

    public String getStableVersion() { return stableVersion; }
    public void setStableVersion(String value) { stableVersion = value; }
    public String getBetaVersion() { return betaVersion; }
    public void setBetaVersion(String value) { betaVersion = value; }
    public String getMinimumSupportedVersion() { return minimumSupportedVersion; }
    public void setMinimumSupportedVersion(String value) { minimumSupportedVersion = value; }
    public String getPublishedAt() { return publishedAt; }
    public void setPublishedAt(String value) { publishedAt = value; }
    public String getStableReleaseNotes() { return stableReleaseNotes; }
    public void setStableReleaseNotes(String value) { stableReleaseNotes = value; }
    public String getBetaReleaseNotes() { return betaReleaseNotes; }
    public void setBetaReleaseNotes(String value) { betaReleaseNotes = value; }
    public boolean isStableEnabled() { return stableEnabled; }
    public void setStableEnabled(boolean value) { stableEnabled = value; }
    public boolean isBetaEnabled() { return betaEnabled; }
    public void setBetaEnabled(boolean value) { betaEnabled = value; }
    public int getStableRolloutPercentage() { return stableRolloutPercentage; }
    public void setStableRolloutPercentage(int value) { stableRolloutPercentage = value; }
    public int getBetaRolloutPercentage() { return betaRolloutPercentage; }
    public void setBetaRolloutPercentage(int value) { betaRolloutPercentage = value; }
    public String getSignatureAlgorithm() { return signatureAlgorithm; }
    public void setSignatureAlgorithm(String value) { signatureAlgorithm = value; }
    public String getPublicKeyVersion() { return publicKeyVersion; }
    public void setPublicKeyVersion(String value) { publicKeyVersion = value; }
    public String getWindowsX64UrlTemplate() { return windowsX64UrlTemplate; }
    public void setWindowsX64UrlTemplate(String value) { windowsX64UrlTemplate = value; }
    public long getWindowsX64Size() { return windowsX64Size; }
    public void setWindowsX64Size(long value) { windowsX64Size = value; }
    public String getWindowsX64Sha256() { return windowsX64Sha256; }
    public void setWindowsX64Sha256(String value) { windowsX64Sha256 = value; }
    public String getWindowsX64SignatureUrl() { return windowsX64SignatureUrl; }
    public void setWindowsX64SignatureUrl(String value) { windowsX64SignatureUrl = value; }
    public String getLinuxX64UrlTemplate() { return linuxX64UrlTemplate; }
    public void setLinuxX64UrlTemplate(String value) { linuxX64UrlTemplate = value; }
    public long getLinuxX64Size() { return linuxX64Size; }
    public void setLinuxX64Size(long value) { linuxX64Size = value; }
    public String getLinuxX64Sha256() { return linuxX64Sha256; }
    public void setLinuxX64Sha256(String value) { linuxX64Sha256 = value; }
    public String getLinuxX64SignatureUrl() { return linuxX64SignatureUrl; }
    public void setLinuxX64SignatureUrl(String value) { linuxX64SignatureUrl = value; }
}
