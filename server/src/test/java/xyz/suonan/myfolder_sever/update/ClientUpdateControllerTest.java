package xyz.suonan.myfolder_sever.update;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.setup.MockMvcBuilders;

import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

class ClientUpdateControllerTest {
    private MockMvc mvc;

    @BeforeEach
    void setUp() {
        ClientUpdateProperties properties = new ClientUpdateProperties();
        properties.setStableVersion("1.1.2");
        properties.setMinimumSupportedVersion("1.1.1");
        properties.setWindowsX64UrlTemplate("https://downloads.example/MyFolder-v{version}-Setup.exe");
        properties.setWindowsX64Size(2048);
        properties.setWindowsX64Sha256("a".repeat(64));
        mvc = MockMvcBuilders.standaloneSetup(
                new ClientUpdateController(new ClientUpdateService(properties))).build();
    }

    @Test
    void returnsPublicManifestContract() throws Exception {
        mvc.perform(get("/api/v1/updates/desktop")
                        .param("platform", "windows")
                        .param("arch", "x64")
                        .param("channel", "stable")
                        .param("currentVersion", "1.1.1")
                        .param("installationId", "install-a"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.latestVersion").value("1.1.2"))
                .andExpect(jsonPath("$.updateAvailable").value(true))
                .andExpect(jsonPath("$.downloadUrl").isString())
                .andExpect(jsonPath("$.sha256").value("a".repeat(64)));
    }

    @Test
    void rejectsUnknownPlatformWithStableErrorCode() throws Exception {
        mvc.perform(get("/api/v1/updates/desktop")
                        .param("platform", "android")
                        .param("currentVersion", "1.1.1"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("INVALID_UPDATE_REQUEST"))
                .andExpect(jsonPath("$.message").isNotEmpty());
    }

    @Test
    void rejectsUnknownProtocolVersionWithStableErrorCode() throws Exception {
        mvc.perform(get("/api/v1/updates/desktop")
                        .param("platform", "windows")
                        .param("currentVersion", "1.1.1")
                        .param("protocolVersion", "99"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("INVALID_UPDATE_REQUEST"));
    }
}
