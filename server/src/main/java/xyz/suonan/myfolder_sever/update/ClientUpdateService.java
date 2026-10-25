package xyz.suonan.myfolder_sever.update;

import org.springframework.stereotype.Service;

import java.net.URI;
import java.util.Locale;

@Service
public class ClientUpdateService {
    private final ClientUpdateProperties properties;

    public ClientUpdateService(ClientUpdateProperties properties) {
        this.properties = properties;
    }

    public ClientUpdateManifest manifest(String platformValue, String architectureValue,
                                         String channelValue, String currentVersionValue) {
        return manifest(platformValue, architectureValue, channelValue, currentVersionValue, "");
    }

    public ClientUpdateManifest manifest(String platformValue, String architectureValue,
                                         String channelValue, String currentVersionValue,
                                         String installationId) {
        String platform = normalizePlatform(platformValue);
        String architecture = normalizeArchitecture(architectureValue);
        String channel = normalizeChannel(channelValue);
        String currentVersion = requireVersion(currentVersionValue, "currentVersion");
        String latestVersion = requireVersion(
                "beta".equals(channel) ? properties.getBetaVersion() : properties.getStableVersion(),
                channel + " version");
        String minimumVersion = requireVersion(properties.getMinimumSupportedVersion(), "minimum supported version");

        Artifact artifact = artifact(platform, architecture, latestVersion);
        boolean enabled = "beta".equals(channel) ? properties.isBetaEnabled() : properties.isStableEnabled();
        int rolloutPercentage = clampPercentage("beta".equals(channel)
                ? properties.getBetaRolloutPercentage() : properties.getStableRolloutPercentage());
        boolean rolloutEligible = installationId == null || installationId.isBlank()
                || Math.floorMod((installationId.trim() + "|" + latestVersion).hashCode(), 100) < rolloutPercentage;
        boolean belowMinimum = compareVersions(currentVersion, minimumVersion) < 0;
        boolean versionIsNewer = compareVersions(currentVersion, latestVersion) < 0;
        // A minimum-version security update overrides gradual rollout, but a
        // withdrawn channel always remains unavailable.
        boolean available = enabled && versionIsNewer && (belowMinimum || rolloutEligible);
        boolean mandatory = enabled && versionIsNewer && belowMinimum;
        String notes = "beta".equals(channel)
                ? properties.getBetaReleaseNotes() : properties.getStableReleaseNotes();

        return new ClientUpdateManifest(1, platform, architecture, channel, currentVersion,
                latestVersion, minimumVersion, available, mandatory, !enabled,
                rolloutPercentage, rolloutEligible,
                safe(properties.getPublishedAt()), safe(notes), artifact.url(), artifact.fileName(),
                artifact.size(), artifact.sha256(), artifact.signatureUrl(),
                safe(properties.getSignatureAlgorithm()), safe(properties.getPublicKeyVersion()), "", "");
    }

    static int compareVersions(String left, String right) {
        int[] a = parseVersion(left);
        int[] b = parseVersion(right);
        for (int i = 0; i < Math.max(a.length, b.length); i++) {
            int av = i < a.length ? a[i] : 0;
            int bv = i < b.length ? b[i] : 0;
            if (av != bv) return Integer.compare(av, bv);
        }
        return 0;
    }

    private Artifact artifact(String platform, String architecture, String version) {
        if (!"x86_64".equals(architecture)) {
            throw new IllegalArgumentException("Unsupported desktop architecture: " + architecture);
        }
        String template;
        long size;
        String sha256;
        String signatureUrl;
        if ("windows".equals(platform)) {
            template = properties.getWindowsX64UrlTemplate();
            size = properties.getWindowsX64Size();
            sha256 = properties.getWindowsX64Sha256();
            signatureUrl = properties.getWindowsX64SignatureUrl();
        } else {
            template = properties.getLinuxX64UrlTemplate();
            size = properties.getLinuxX64Size();
            sha256 = properties.getLinuxX64Sha256();
            signatureUrl = properties.getLinuxX64SignatureUrl();
        }
        String url = safe(template).replace("{version}", version);
        if (!url.isBlank()) {
            URI uri = URI.create(url);
            if (!"https".equalsIgnoreCase(uri.getScheme())) {
                throw new IllegalStateException("Desktop update URL must use HTTPS");
            }
        }
        String fileName = url.isBlank() ? "" : url.substring(url.lastIndexOf('/') + 1);
        return new Artifact(url, fileName, Math.max(0, size), safe(sha256).toLowerCase(Locale.ROOT), safe(signatureUrl));
    }

    private static String normalizePlatform(String value) {
        String normalized = safe(value).toLowerCase(Locale.ROOT);
        if (normalized.equals("win") || normalized.equals("win32")) normalized = "windows";
        if (!normalized.equals("windows") && !normalized.equals("linux")) {
            throw new IllegalArgumentException("Unsupported desktop platform: " + value);
        }
        return normalized;
    }

    private static String normalizeArchitecture(String value) {
        String normalized = safe(value).toLowerCase(Locale.ROOT).replace('-', '_');
        if (normalized.equals("amd64") || normalized.equals("x64")) normalized = "x86_64";
        return normalized;
    }

    private static String normalizeChannel(String value) {
        String normalized = safe(value).toLowerCase(Locale.ROOT);
        if (normalized.isBlank()) normalized = "stable";
        if (!normalized.equals("stable") && !normalized.equals("beta")) {
            throw new IllegalArgumentException("Unsupported update channel: " + value);
        }
        return normalized;
    }

    private static String requireVersion(String value, String label) {
        String normalized = safe(value);
        parseVersion(normalized);
        if (normalized.startsWith("v")) normalized = normalized.substring(1);
        return normalized;
    }

    private static int[] parseVersion(String value) {
        String normalized = safe(value);
        if (normalized.startsWith("v")) normalized = normalized.substring(1);
        String numeric = normalized.split("[-+]", 2)[0];
        if (!numeric.matches("\\d+(\\.\\d+){1,3}")) {
            throw new IllegalArgumentException("Invalid version: " + value);
        }
        String[] parts = numeric.split("\\.");
        int[] parsed = new int[parts.length];
        for (int i = 0; i < parts.length; i++) parsed[i] = Integer.parseInt(parts[i]);
        return parsed;
    }

    private static String safe(String value) { return value == null ? "" : value.trim(); }

    private static int clampPercentage(int value) { return Math.max(0, Math.min(100, value)); }

    private record Artifact(String url, String fileName, long size, String sha256, String signatureUrl) { }
}
