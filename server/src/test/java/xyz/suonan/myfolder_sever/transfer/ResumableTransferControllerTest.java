package xyz.suonan.myfolder_sever.transfer;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import xyz.suonan.myfolder_sever.transfer.api.CreateUploadTaskRequest;
import xyz.suonan.myfolder_sever.transfer.api.ResumableTransferController;
import xyz.suonan.myfolder_sever.transfer.error.TransferErrorCode;
import xyz.suonan.myfolder_sever.transfer.error.TransferException;
import xyz.suonan.myfolder_sever.transfer.error.TransferExceptionHandler;
import xyz.suonan.myfolder_sever.transfer.model.UploadFile;
import xyz.suonan.myfolder_sever.transfer.model.UploadState;
import xyz.suonan.myfolder_sever.transfer.model.UploadTask;
import xyz.suonan.myfolder_sever.transfer.service.ResumableTransferService;

import java.time.Instant;
import java.util.List;
import java.util.Map;

import static org.mockito.Mockito.*;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;
import static org.springframework.test.web.servlet.setup.MockMvcBuilders.standaloneSetup;

class ResumableTransferControllerTest {
    private static final String BASE = "/file/uploadfile";
    private static final String USER = "alice";
    private static final String UPLOAD_ID = "AGMXDF5R7V6TBFWFGGRBJM7AL4";
    private static final String HASH = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

    private ResumableTransferService service;
    private ObjectMapper objectMapper;
    private MockMvc mockMvc;
    private UploadTask task;

    @BeforeEach
    void setUp() {
        service = mock(ResumableTransferService.class);
        objectMapper = new ObjectMapper().registerModule(new JavaTimeModule());
        mockMvc = standaloneSetup(new ResumableTransferController(service))
                .setControllerAdvice(new TransferExceptionHandler())
                .setMessageConverters(
                        new org.springframework.http.converter.ByteArrayHttpMessageConverter(),
                        new org.springframework.http.converter.json.MappingJackson2HttpMessageConverter(objectMapper))
                .build();
        task = task();
    }

    @Test
    void createRouteReturns201AndProtocolResponse() throws Exception {
        CreateUploadTaskRequest request = new CreateUploadTaskRequest("inbox/demo", 65_536,
                1, 5, List.of(new CreateUploadTaskRequest.FileManifest("docs/a.txt", 5, HASH, 1)));
        when(service.create(eq(USER), any())).thenReturn(task);

        mockMvc.perform(post(BASE).requestAttr("myfolder.userId", USER).contentType(MediaType.APPLICATION_JSON)
                        .content(objectMapper.writeValueAsBytes(request)))
                .andExpect(status().isCreated())
                .andExpect(jsonPath("$.uploadId").value(UPLOAD_ID))
                .andExpect(jsonPath("$.targetPath").value("inbox/demo"))
                .andExpect(jsonPath("$.state").value("UPLOADING"))
                .andExpect(jsonPath("$.files[0].missingChunks[0]").value(0));
    }

    @Test
    void fileUploadRouteUsesTheSameResumableAndHashCheckedProtocol() throws Exception {
        CreateUploadTaskRequest request = new CreateUploadTaskRequest("inbox/demo", 65_536,
                1, 5, List.of(new CreateUploadTaskRequest.FileManifest("docs/a.txt", 5, HASH, 1)));
        when(service.create(eq(USER), any())).thenReturn(task);
        when(service.uploadChunk(USER, UPLOAD_ID, "docs/a.txt", 0, 0, 4, 5, HASH,
                "hello".getBytes(java.nio.charset.StandardCharsets.UTF_8))).thenReturn(task);

        mockMvc.perform(post(BASE).requestAttr("myfolder.userId", USER)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content(objectMapper.writeValueAsBytes(request)))
                .andExpect(status().isCreated())
                .andExpect(jsonPath("$.uploadId").value(UPLOAD_ID))
                .andExpect(jsonPath("$.files[0].missingChunks[0]").value(0));

        mockMvc.perform(put(BASE + "/" + UPLOAD_ID + "/chunks/0")
                        .requestAttr("myfolder.userId", USER)
                        .contentType(MediaType.APPLICATION_OCTET_STREAM)
                        .header("X-File-Path", "docs%2Fa.txt")
                        .header("X-Chunk-SHA256", HASH)
                        .header("Content-Range", "bytes 0-4/5")
                        .content("hello"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.uploadId").value(UPLOAD_ID));

        verify(service).create(eq(USER), any());
        verify(service).uploadChunk(USER, UPLOAD_ID, "docs/a.txt", 0, 0, 4, 5, HASH,
                "hello".getBytes(java.nio.charset.StandardCharsets.UTF_8));
    }

    @Test
    void legacyTransferRouteStillSupportsDesktopRelayUploads() throws Exception {
        when(service.create(eq(USER), any())).thenReturn(task);

        mockMvc.perform(post("/api/v1/transfers/tasks").requestAttr("myfolder.userId", USER)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"directoryName":"demo","parentPath":"relay","chunkSize":65536,
                                 "totalFiles":1,"totalBytes":5,
                                 "files":[{"path":"docs/a.txt","size":5,"sha256":"%s","totalChunks":1}]}
                                """.formatted(HASH)))
                .andExpect(status().isCreated())
                .andExpect(jsonPath("$.uploadId").value(UPLOAD_ID));

        verify(service).create(eq(USER), argThat(request -> "relay/demo".equals(request.targetPath())));
    }

    @Test
    void statusRouteUsesServerTaskAsAuthority() throws Exception {
        when(service.status(USER, UPLOAD_ID)).thenReturn(task);

        mockMvc.perform(get(BASE + "/" + UPLOAD_ID).requestAttr("myfolder.userId", USER))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.files[0].completedChunks").isEmpty())
                .andExpect(jsonPath("$.files[0].missingChunks[0]").value(0));
    }

    @Test
    void listRouteReturnsCurrentUsersUploadHistory() throws Exception {
        when(service.list(USER)).thenReturn(List.of(task));

        mockMvc.perform(get(BASE).requestAttr("myfolder.userId", USER))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$[0].uploadId").value(UPLOAD_ID))
                .andExpect(jsonPath("$[0].targetPath").value("inbox/demo"))
                .andExpect(jsonPath("$[0].createdAt").value(1786071600.0));

        verify(service).list(USER);
    }

    @Test
    void chunkRouteDecodesPathAndParsesContentRange() throws Exception {
        byte[] content = "hello".getBytes(java.nio.charset.StandardCharsets.UTF_8);
        when(service.uploadChunk(USER, UPLOAD_ID, "docs/a b.txt", 0, 0, 4, 5, HASH, content)).thenReturn(task);

        mockMvc.perform(put(BASE + "/" + UPLOAD_ID + "/chunks/0")
                        .requestAttr("myfolder.userId", USER)
                        .contentType(MediaType.APPLICATION_OCTET_STREAM)
                        .header("X-File-Path", "docs%2Fa%20b.txt")
                        .header("X-Chunk-SHA256", HASH)
                        .header("Content-Range", "bytes 0-4/5")
                        .content(content))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.uploadId").value(UPLOAD_ID));

        verify(service).uploadChunk(USER, UPLOAD_ID, "docs/a b.txt", 0, 0, 4, 5, HASH, content);
    }

    @Test
    void completeFileRouteUsesFrozenRequestField() throws Exception {
        when(service.completeFile(USER, UPLOAD_ID, "docs/a.txt")).thenReturn(task);

        mockMvc.perform(post(BASE + "/" + UPLOAD_ID + "/files/complete")
                        .requestAttr("myfolder.userId", USER)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"filePath\":\"docs/a.txt\"}"))
                .andExpect(status().isOk());

        verify(service).completeFile(USER, UPLOAD_ID, "docs/a.txt");
    }

    @Test
    void completeAndCancelRoutesMatchTheFrozenProtocol() throws Exception {
        when(service.completeTask(USER, UPLOAD_ID)).thenReturn(task);
        when(service.cancel(USER, UPLOAD_ID)).thenReturn(task);

        mockMvc.perform(post(BASE + "/" + UPLOAD_ID + "/complete").requestAttr("myfolder.userId", USER))
                .andExpect(status().isOk());
        mockMvc.perform(delete(BASE + "/" + UPLOAD_ID).requestAttr("myfolder.userId", USER))
                .andExpect(status().isOk());

        verify(service).completeTask(USER, UPLOAD_ID);
        verify(service).cancel(USER, UPLOAD_ID);
    }

    @Test
    void malformedRangeUsesTheUnifiedErrorEnvelope() throws Exception {
        mockMvc.perform(put(BASE + "/" + UPLOAD_ID + "/chunks/0")
                        .requestAttr("myfolder.userId", USER)
                        .contentType(MediaType.APPLICATION_OCTET_STREAM)
                        .header("X-File-Path", "docs%2Fa.txt")
                        .header("X-Chunk-SHA256", HASH)
                        .header("Content-Range", "0-4/5")
                        .content("hello"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("CHUNK_RANGE_INVALID"))
                .andExpect(jsonPath("$.status").value(400))
                .andExpect(jsonPath("$.details").isMap());
    }

    @Test
    void businessFailurePreservesStableCodeAndDetails() throws Exception {
        when(service.completeFile(USER, UPLOAD_ID, "docs/a.txt")).thenThrow(new TransferException(
                TransferErrorCode.FILE_INCOMPLETE, HttpStatus.CONFLICT, "File still has missing chunks",
                Map.of("missingChunks", List.of(0))));

        mockMvc.perform(post(BASE + "/" + UPLOAD_ID + "/files/complete")
                        .requestAttr("myfolder.userId", USER)
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"filePath\":\"docs/a.txt\"}"))
                .andExpect(status().isConflict())
                .andExpect(jsonPath("$.code").value("FILE_INCOMPLETE"))
                .andExpect(jsonPath("$.details.missingChunks[0]").value(0));
    }

    private UploadTask task() {
        UploadTask result = new UploadTask();
        result.uploadId = UPLOAD_ID;
        result.ownerUserId = USER;
        result.targetPath = "inbox/demo";
        result.chunkSize = 65_536;
        result.totalFiles = 1;
        result.totalBytes = 5;
        result.state = UploadState.UPLOADING;
        result.createdAt = Instant.parse("2026-08-07T03:00:00Z");
        result.updatedAt = result.createdAt;
        result.files.put("docs/a.txt", new UploadFile("docs/a.txt", 5, HASH, 1));
        return result;
    }
}
