package xyz.suonan.myfolder_sever.file;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import xyz.suonan.myfolder_sever.MyObject.FileInfo;
import xyz.suonan.myfolder_sever.Service.FileInfoService;
import xyz.suonan.myfolder_sever.Utils.WrapFileBaseItem;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.*;

class FileStorageServiceTest {
    @TempDir Path root;
    private FileInfoService metadata;
    private FileStorageService service;

    @BeforeEach
    void setUp() {
        metadata = mock(FileInfoService.class);
        service = new FileStorageService(new StoragePathResolver(root.toString()),
                new WrapFileBaseItem(root.toString()), metadata);
    }

    @Test
    void deletesSingleFileWithoutDirectoryWalkFailure() throws Exception {
        Path file = Files.writeString(root.resolve("single.txt"), "hello");

        service.delete("single.txt");

        assertFalse(Files.exists(file));
        verify(metadata).deleteFileByPath(file.toAbsolutePath().toString());
    }

    @Test
    void metadataFailureAfterDeletionReturnsStructuredReason() throws Exception {
        Path file = Files.writeString(root.resolve("single.txt"), "hello");
        doThrow(new IllegalStateException("database unavailable"))
                .when(metadata).deleteFileByPath(anyString());

        FileOperationException exception = assertThrows(FileOperationException.class,
                () -> service.delete("single.txt"));
        assertEquals("METADATA_ERROR", exception.getCode());
        assertEquals("DELETE", exception.getDetails().get("operation"));
        assertTrue(exception.getDetails().get("reason").toString().contains("database unavailable"));
        assertFalse(Files.exists(file));
    }

    @Test
    void metadataFailureAfterCopyReturnsStructuredReason() throws Exception {
        doThrow(new IllegalStateException("file_info table unavailable"))
                .when(metadata).insertFileInfo(any(FileInfo.class));
        Files.writeString(root.resolve("source.txt"), "hello");

        FileOperationException exception = assertThrows(FileOperationException.class,
                () -> service.copy("source.txt", "copy.txt"));
        assertEquals("METADATA_ERROR", exception.getCode());
        assertEquals("WRITE", exception.getDetails().get("operation"));
        assertTrue(exception.getDetails().get("reason").toString().contains("file_info table unavailable"));
        assertTrue(Files.exists(root.resolve("copy.txt")));
    }

    @Test
    void recursivelyDeletesNonEmptyDirectory() throws Exception {
        Path directory = Files.createDirectories(root.resolve("folder/child"));
        Files.writeString(directory.resolve("one.txt"), "one");
        Files.writeString(root.resolve("folder/two.txt"), "two");

        service.delete("folder");

        assertFalse(Files.exists(root.resolve("folder")));
        verify(metadata, times(2)).deleteFileByPath(anyString());
    }

    @Test
    void rejectsTraversalOutsideStorageRoot() {
        FileOperationException exception = assertThrows(FileOperationException.class,
                () -> service.delete("../../outside.txt"));
        assertEquals("INVALID_PATH", exception.getCode());
        assertEquals(400, exception.getStatus().value());
    }

    @Test
    void listsAndDownloadsPublicFilesInsideRoot() throws Exception {
        Files.writeString(root.resolve("public.txt"), "public");

        assertEquals(1, service.list("").size());
        assertTrue(service.download("public.txt").isReadable());
    }

    @Test
    void hidesAndProtectsInternalMyfolderDirectory() throws Exception {
        Path internalFile = root.resolve(".myfolder/staging/task/file.part");
        Files.createDirectories(internalFile.getParent());
        Files.writeString(internalFile, "partial upload");
        Files.writeString(root.resolve("visible.txt"), "public");

        assertEquals(List.of("visible.txt"), service.list("").stream().map(item -> item.getName()).toList());

        FileOperationException listError = assertThrows(FileOperationException.class,
                () -> service.list(".myfolder"));
        assertEquals("RESERVED_PATH", listError.getCode());
        assertEquals(403, listError.getStatus().value());

        FileOperationException deleteError = assertThrows(FileOperationException.class,
                () -> service.delete(".myfolder/staging"));
        assertEquals("RESERVED_PATH", deleteError.getCode());
        assertTrue(Files.exists(internalFile));
    }

    @Test
    void recursivelyCopiesDirectory() throws Exception {
        Files.createDirectories(root.resolve("source/child"));
        Files.writeString(root.resolve("source/child/file.txt"), "content");

        service.copy("source", "copy");

        assertEquals("content", Files.readString(root.resolve("copy/child/file.txt")));
    }
}
