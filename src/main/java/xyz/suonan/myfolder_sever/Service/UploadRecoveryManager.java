package xyz.suonan.myfolder_sever.Service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;

@Service
public class UploadRecoveryManager {

    @Autowired
    DirectoryInfoService directoryInfoService;
    @Autowired
    DirectoryFileService directoryFileService;
    @Value("${basePath}")
    private String basePath;

    private static void deleteDirectory(Path path) throws IOException {
        if (!Files.exists(path)) return;

        Files.walkFileTree(path, new SimpleFileVisitor<>() {
            @Override
            public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
                Files.delete(file);
                return FileVisitResult.CONTINUE;
            }

            @Override
            public FileVisitResult postVisitDirectory(Path dir, IOException exc) throws IOException {
                Files.delete(dir);
                return FileVisitResult.CONTINUE;
            }
        });
    }

    public void Recover(String uploadId) throws IOException {
        String parentPath=directoryInfoService.getParentNameById(uploadId);
        String directoryName=directoryInfoService.getDirectoryNameById(uploadId);
        Path absolutePath= Paths.get(basePath,parentPath,directoryName);
        deleteDirectory(absolutePath);
        directoryFileService.deleteDirectoryFile(uploadId);
        directoryInfoService.deleteDirectoryInfo(uploadId);
        //TODO::加对filInfo的回滚


    }



}
