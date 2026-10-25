package xyz.suonan.myfolder_sever.update;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

class ClientUpdateServiceTest {
    private ClientUpdateProperties properties;
    private ClientUpdateService service;

    @BeforeEach
    void setUp() {
        properties = new ClientUpdateProperties();
        properties.setStableVersion("1.1.2");
        properties.setBetaVersion("1.1.3");
        properties.setMinimumSupportedVersion("1.1.1");
        properties.setWindowsX64UrlTemplate("https://downloads.example/MyFolder-v{version}-Setup.exe");
        properties.setWindowsX64Size(1234);
        properties.setWindowsX64Sha256("AABB");
        properties.setLinuxX64UrlTemplate("https://downloads.example/myfolder_{version}_amd64.deb");
        service = new ClientUpdateService(properties);
    }

    @Test
    void reportsStableWindowsUpdateAndExpandsVersionUrl() {
        ClientUpdateManifest result = service.manifest("win32", "amd64", "stable", "1.1.1");
        assertTrue(result.updateAvailable());
        assertFalse(result.mandatory());
        assertEquals("windows", result.platform());
        assertEquals("x86_64", result.architecture());
        assertEquals("1.1.2", result.latestVersion());
        assertEquals("https://downloads.example/MyFolder-v1.1.2-Setup.exe", result.downloadUrl());
        assertEquals("aabb", result.sha256());
    }

    @Test
    void marksClientBelowMinimumAsMandatory() {
        ClientUpdateManifest result = service.manifest("linux", "x64", "stable", "1.0.9");
        assertTrue(result.updateAvailable());
        assertTrue(result.mandatory());
    }

    @Test
    void doesNotOfferSameOrNewerVersion() {
        assertFalse(service.manifest("windows", "x86_64", "stable", "1.1.2").updateAvailable());
        assertFalse(service.manifest("windows", "x86_64", "stable", "1.2.0").updateAvailable());
    }

    @Test
    void selectsBetaChannelIndependently() {
        assertEquals("1.1.3", service.manifest("windows", "x86_64", "beta", "1.1.2").latestVersion());
    }

    @Test
    void rejectsUnsupportedInputsAndInsecureUrls() {
        assertThrows(IllegalArgumentException.class,
                () -> service.manifest("android", "x86_64", "stable", "1.1.1"));
        assertThrows(IllegalArgumentException.class,
                () -> service.manifest("windows", "arm64", "stable", "1.1.1"));
        assertThrows(IllegalArgumentException.class,
                () -> service.manifest("windows", "x86_64", "nightly", "1.1.1"));
        properties.setWindowsX64UrlTemplate("http://downloads.example/update.exe");
        assertThrows(IllegalStateException.class,
                () -> service.manifest("windows", "x86_64", "stable", "1.1.1"));
    }

    @Test
    void canPauseAndStageAChannel() {
        properties.setStableEnabled(false);
        ClientUpdateManifest paused = service.manifest("windows", "x64", "stable", "1.1.1", "device-a");
        assertTrue(paused.withdrawn());
        assertFalse(paused.updateAvailable());

        properties.setStableEnabled(true);
        properties.setStableRolloutPercentage(0);
        ClientUpdateManifest staged = service.manifest("windows", "x64", "stable", "1.1.1", "device-a");
        assertFalse(staged.rolloutEligible());
        assertFalse(staged.updateAvailable());
    }

    @Test
    void mandatorySecurityUpdateOverridesGradualRollout() {
        properties.setMinimumSupportedVersion("1.1.2");
        properties.setStableRolloutPercentage(0);
        ClientUpdateManifest result = service.manifest("windows", "x64", "stable", "1.1.1", "device-a");
        assertTrue(result.updateAvailable());
        assertTrue(result.mandatory());
        assertFalse(result.rolloutEligible());
    }
}
