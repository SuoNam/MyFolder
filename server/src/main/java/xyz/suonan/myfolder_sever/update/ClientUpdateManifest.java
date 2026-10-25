package xyz.suonan.myfolder_sever.update;

public record ClientUpdateManifest(
        int protocolVersion,
        String platform,
        String architecture,
        String channel,
        String currentVersion,
        String latestVersion,
        String minimumSupportedVersion,
        boolean updateAvailable,
        boolean mandatory,
        boolean withdrawn,
        int rolloutPercentage,
        boolean rolloutEligible,
        String publishedAt,
        String releaseNotes,
        String downloadUrl,
        String fileName,
        long size,
        String sha256,
        String signatureUrl,
        String signatureAlgorithm,
        String publicKeyVersion,
        String deltaFromVersion,
        String deltaUrl) {
}
