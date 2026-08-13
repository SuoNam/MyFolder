package xyz.suonan.myfolder_sever.file;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

class StoragePathResolverTest {
    @TempDir
    Path root;

    @Test
    void supportsUnicodeOAuthAccountNames() {
        StoragePathResolver resolver = new StoragePathResolver(root.toString());

        Path privateRoot = resolver.privateRoot("索剑坤");

        assertTrue(privateRoot.startsWith(root.resolve("users")));
        assertEquals("索剑坤", privateRoot.getFileName().toString());
    }

    @Test
    void rejectsTraversalSegmentsForIsolatedRoots() {
        StoragePathResolver resolver = new StoragePathResolver(root.toString());

        assertThrows(FileOperationException.class, () -> resolver.privateRoot(".."));
        assertThrows(FileOperationException.class, () -> resolver.privateRoot("a/b"));
        assertThrows(FileOperationException.class, () -> resolver.groupRoot("a\\b"));
    }
}
