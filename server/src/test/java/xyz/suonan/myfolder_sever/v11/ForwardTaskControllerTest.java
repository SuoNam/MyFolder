package xyz.suonan.myfolder_sever.v11;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.MvcResult;
import xyz.suonan.myfolder_sever.device.api.DeviceExceptionHandler;
import xyz.suonan.myfolder_sever.forward.api.ForwardTaskController;
import xyz.suonan.myfolder_sever.forward.model.ForwardChannel;
import xyz.suonan.myfolder_sever.forward.model.ForwardState;
import xyz.suonan.myfolder_sever.forward.model.ForwardTask;
import xyz.suonan.myfolder_sever.forward.service.ForwardTaskService;

import java.time.Instant;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;
import static org.springframework.test.web.servlet.setup.MockMvcBuilders.standaloneSetup;

class ForwardTaskControllerTest {
    private static final String USER = "alice";
    private static final String HASH = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

    private ForwardTaskService service;
    private MockMvc mockMvc;

    @BeforeEach
    void setUp() {
        service = mock(ForwardTaskService.class);
        ObjectMapper mapper = new ObjectMapper().registerModule(new JavaTimeModule());
        mockMvc = standaloneSetup(new ForwardTaskController(service))
                .setControllerAdvice(new DeviceExceptionHandler())
                .setMessageConverters(new org.springframework.http.converter.json.MappingJackson2HttpMessageConverter(mapper))
                .build();
    }

    @Test
    void createRequiresAndPassesDeviceCredentials() throws Exception {
        ForwardTask task = task();
        when(service.create(eq(USER), eq("source"), eq("token"), any())).thenReturn(task);

        mockMvc.perform(post("/api/v1/forwards")
                        .requestAttr("myfolder.userId", USER)
                        .header("X-Device-Id", "source")
                        .header("X-Device-Token", "token")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content(validJson()))
                .andExpect(status().isCreated())
                .andExpect(jsonPath("$.forwardId").value("forward-1"));

        verify(service).create(eq(USER), eq("source"), eq("token"), any());
    }

    @Test
    void missingDeviceCredentialUsesUnifiedErrorEnvelope() throws Exception {
        mockMvc.perform(post("/api/v1/forwards")
                        .requestAttr("myfolder.userId", USER)
                        .header("X-Device-Id", "source")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content(validJson()))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.status").value(400))
                .andExpect(jsonPath("$.code").value("INVALID_REQUEST"))
                .andExpect(jsonPath("$.details").isMap());
    }

    @Test
    void rejectsUploadOnlyFieldsInForwardManifest() throws Exception {
        String extraField = validJson().replace("\"sha256\":\"" + HASH + "\"",
                "\"sha256\":\"" + HASH + "\",\"progress\":50");

        mockMvc.perform(post("/api/v1/forwards")
                        .requestAttr("myfolder.userId", USER)
                        .header("X-Device-Id", "source")
                        .header("X-Device-Token", "token")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content(extraField))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("INVALID_REQUEST"));
    }

    @Test
    void relayDownloadStreamsTheRequestedRangeAndHeaders(@TempDir Path temp) throws Exception {
        Path file = temp.resolve("a.txt");
        Files.writeString(file, "hello");
        when(service.relayContent(USER, "target", "token", "forward-1", "a.txt", "bytes=1-3"))
                .thenReturn(new ForwardTaskService.RelayContent(file, "a.txt", HASH, 5, 1, 3, true));

        MvcResult pending = mockMvc.perform(get("/api/v1/forwards/forward-1/files/content")
                        .requestAttr("myfolder.userId", USER)
                        .queryParam("path", "a.txt")
                        .header("Range", "bytes=1-3")
                        .header("X-Device-Id", "target")
                        .header("X-Device-Token", "token"))
                .andExpect(request().asyncStarted())
                .andReturn();

        mockMvc.perform(asyncDispatch(pending))
                .andExpect(status().isPartialContent())
                .andExpect(header().string("Accept-Ranges", "bytes"))
                .andExpect(header().string("Content-Range", "bytes 1-3/5"))
                .andExpect(header().string("Content-Length", "3"))
                .andExpect(header().string("X-File-SHA256", HASH))
                .andExpect(content().bytes("ell".getBytes(java.nio.charset.StandardCharsets.UTF_8)));
    }

    private String validJson() {
        return "{\"sourceDeviceId\":\"source\",\"targetDeviceId\":\"target\"," +
                "\"destinationPath\":\"Downloads\",\"deleteSource\":false,\"channel\":\"LAN\"," +
                "\"relayUploadId\":null,\"files\":[{\"path\":\"a.txt\",\"size\":5," +
                "\"sha256\":\"" + HASH + "\"}]}";
    }

    private ForwardTask task() {
        ForwardTask task = new ForwardTask();
        task.setForwardId("forward-1");
        task.setUserId(USER);
        task.setSourceDeviceId("source");
        task.setTargetDeviceId("target");
        task.setDestinationPath("Downloads");
        task.setChannel(ForwardChannel.LAN);
        task.setFiles(List.of());
        task.setState(ForwardState.OFFERED);
        task.setCreatedAt(Instant.now());
        task.setUpdatedAt(task.getCreatedAt());
        return task;
    }
}
