package xyz.suonan.myfolder_sever.file;

import org.springframework.core.io.Resource;
import org.springframework.core.io.UrlResource;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.MyObject.FileInfo;
import xyz.suonan.myfolder_sever.MyObject.Item.FileBaseItem;
import xyz.suonan.myfolder_sever.MyObject.Item.DirectoryItem;
import xyz.suonan.myfolder_sever.MyObject.Item.FileInfoItem;
import xyz.suonan.myfolder_sever.Service.FileInfoService;
import xyz.suonan.myfolder_sever.Utils.FileHashUtil;
import xyz.suonan.myfolder_sever.Utils.WrapFileBaseItem;

import java.io.File;
import java.io.IOException;
import java.nio.file.*;
import java.util.Comparator;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

@Service
public class FileStorageService {
    private final StoragePathResolver paths;
    private final WrapFileBaseItem itemWrapper;
    private final FileInfoService fileInfoService;

    public FileStorageService(StoragePathResolver paths, WrapFileBaseItem itemWrapper,
                              FileInfoService fileInfoService) {
        this.paths = paths;
        this.itemWrapper = itemWrapper;
        this.fileInfoService = fileInfoService;
    }

    public List<FileBaseItem> list(String directoryPath) {
        return list(paths.root(), directoryPath);
    }

    public List<FileBaseItem> list(Path scopeRoot, String directoryPath) {
        Path directory = paths.resolveExisting(scopeRoot, directoryPath == null ? "" : directoryPath);
        if (!Files.isDirectory(directory)) throw badRequest("NOT_A_DIRECTORY", "请求路径不是目录", directoryPath);
        try (Stream<Path> children = Files.list(directory)) {
            return children.filter(path -> !paths.isInternal(path))
                    .sorted(Comparator.comparing(path -> path.getFileName().toString(),
                            String.CASE_INSENSITIVE_ORDER))
                    .map(path -> wrap(scopeRoot, path))
                    .filter(java.util.Objects::nonNull)
                    .toList();
        } catch (IOException exception) {
            throw io("读取目录", directoryPath, exception);
        }
    }

    public Resource download(String filePath) {
        return download(paths.root(), filePath);
    }

    public Resource download(Path scopeRoot, String filePath) {
        Path file = paths.resolveExisting(scopeRoot, filePath);
        if (!Files.isRegularFile(file)) throw badRequest("NOT_A_FILE", "请求路径不是文件", filePath);
        try {
            Resource resource = new UrlResource(file.toUri());
            if (!resource.isReadable()) throw new AccessDeniedException(file.toString());
            return resource;
        } catch (IOException exception) {
            throw io("下载文件", filePath, exception);
        }
    }


    public void createFolder(String path) {
        createFolder(paths.root(), path);
    }

    public void createFolder(Path scopeRoot, String path) {
        Path directory = paths.resolve(scopeRoot, path);
        try {
            Files.createDirectories(directory);
        } catch (IOException exception) {
            throw io("创建目录", path, exception);
        }
    }

    public void move(String sourcePath, String targetPath) {
        move(paths.root(), sourcePath, targetPath);
    }

    public void move(Path scopeRoot, String sourcePath, String targetPath) {
        Path source = paths.resolveExisting(scopeRoot, sourcePath);
        Path target = paths.resolve(scopeRoot, targetPath);
        boolean sourceFile = Files.isRegularFile(source);
        List<Path> oldFiles = sourceFile ? List.of(source) : regularFiles(source);
        try {
            Files.move(source, target, StandardCopyOption.REPLACE_EXISTING);
            for (Path oldFile : oldFiles) {
                Path relative = sourceFile ? Path.of("") : source.relativize(oldFile);
                Path newFile = sourceFile ? target : target.resolve(relative);
                updateMetadata(oldFile, newFile);
            }
        } catch (IOException exception) {
            throw io("移动文件", sourcePath, exception);
        }
    }

    public void copy(String sourcePath, String targetPath) {
        copy(paths.root(), sourcePath, targetPath);
    }

    public void copy(Path scopeRoot, String sourcePath, String targetPath) {
        Path source = paths.resolveExisting(scopeRoot, sourcePath);
        Path target = paths.resolve(scopeRoot, targetPath);
        if (Files.isDirectory(source) && target.startsWith(source)) {
            throw badRequest("INVALID_TARGET", "不能把目录复制到自身内部", targetPath);
        }
        try {
            if (Files.isRegularFile(source)) {
                Files.copy(source, target, StandardCopyOption.REPLACE_EXISTING);
                recordMetadata(target);
                return;
            }
            List<Path> entries;
            try (Stream<Path> walk = Files.walk(source)) { entries = walk.toList(); }
            for (Path entry : entries) {
                Path destination = target.resolve(source.relativize(entry));
                if (Files.isDirectory(entry)) Files.createDirectories(destination);
                else {
                    Files.copy(entry, destination, StandardCopyOption.REPLACE_EXISTING);
                    recordMetadata(destination);
                }
            }
        } catch (IOException exception) {
            throw io("复制文件", sourcePath, exception);
        }
    }

    public void delete(String sourcePath) {
        delete(paths.root(), sourcePath);
    }

    public void delete(Path scopeRoot, String sourcePath) {
        Path source = paths.resolveExisting(scopeRoot, sourcePath);
        try {
            if (Files.isRegularFile(source, LinkOption.NOFOLLOW_LINKS) || Files.isSymbolicLink(source)) {
                deleteOne(source);
                return;
            }
            List<Path> entries;
            try (Stream<Path> walk = Files.walk(source)) {
                entries = walk.sorted(Comparator.reverseOrder()).toList();
            }
            for (Path entry : entries) deleteOne(entry);
        } catch (IOException exception) {
            throw io("删除文件", sourcePath, exception);
        }
    }

    private void deleteOne(Path path) throws IOException {
        boolean file = Files.isRegularFile(path, LinkOption.NOFOLLOW_LINKS);
        Files.delete(path);
        if (file) deleteMetadata(path);
    }

    private List<Path> regularFiles(Path source) {
        try (Stream<Path> walk = Files.walk(source)) {
            return walk.filter(Files::isRegularFile).toList();
        } catch (IOException exception) {
            throw io("读取目录", paths.relative(source), exception);
        }
    }

    public long totalSize(Path scopeRoot, String sourcePath) {
        Path source = paths.resolveExisting(scopeRoot, sourcePath);
        try {
            if (Files.isRegularFile(source)) return Files.size(source);
            try (Stream<Path> walk = Files.walk(source)) {
                return walk.filter(Files::isRegularFile).mapToLong(p -> {
                    try { return Files.size(p); } catch (IOException e) { throw new java.io.UncheckedIOException(e); }
                }).sum();
            }
        } catch (IOException | java.io.UncheckedIOException exception) {
            throw io("统计文件大小", sourcePath, exception instanceof java.io.UncheckedIOException u ? u.getCause() : (IOException) exception);
        }
    }

    private FileBaseItem wrap(Path scopeRoot, Path path) {
        File file = path.toFile();
        Path relative = Path.of(paths.relative(scopeRoot, path));
        if (file.isDirectory()) {
            return new DirectoryItem(file.getName(), "Directory", relative, new java.util.Date(file.lastModified()));
        }
        if (file.isFile()) {
            return new FileInfoItem("File", file.getName(), file.length(), new java.util.Date(file.lastModified()), relative);
        }
        return null;
    }

    private void recordMetadata(Path file) {
        try {
            File destination = file.toFile();
            fileInfoService.insertFileInfo(new FileInfo(destination.getAbsolutePath(), destination.length(),
                    destination.lastModified(), HexFormat.of().parseHex(FileHashUtil.calculateSHA256(destination))));
        } catch (RuntimeException exception) {
            throw metadata("WRITE", file, exception);
        }
    }

    private void deleteMetadata(Path file) {
        try {
            fileInfoService.deleteFileByPath(file.toAbsolutePath().toString());
        } catch (RuntimeException exception) {
            throw metadata("DELETE", file, exception);
        }
    }

    private void updateMetadata(Path source, Path target) {
        try {
            fileInfoService.updateFileInfoByPath(source.toAbsolutePath().toString(), target.toAbsolutePath().toString());
        } catch (RuntimeException exception) {
            throw new FileOperationException(HttpStatus.INTERNAL_SERVER_ERROR, "METADATA_ERROR",
                    "文件已移动，但更新文件元数据失败", exception, Map.of(
                    "operation", "MOVE",
                    "sourcePath", paths.relative(source),
                    "targetPath", paths.relative(target),
                    "reason", rootCauseMessage(exception)));
        }
    }

    private FileOperationException metadata(String operation, Path file, RuntimeException exception) {
        String message = switch (operation) {
            case "WRITE" -> "文件已写入，但记录文件元数据失败";
            case "DELETE" -> "文件已删除，但清理文件元数据失败";
            default -> "文件元数据操作失败";
        };
        return new FileOperationException(HttpStatus.INTERNAL_SERVER_ERROR, "METADATA_ERROR", message,
                exception, Map.of(
                "operation", operation,
                "path", paths.relative(file),
                "reason", rootCauseMessage(exception)));
    }

    private String rootCauseMessage(Throwable exception) {
        Throwable root = exception;
        while (root.getCause() != null && root.getCause() != root) root = root.getCause();
        String message = root.getMessage();
        return root.getClass().getSimpleName() + (message == null || message.isBlank() ? "" : ": " + message);
    }

    private FileOperationException badRequest(String code, String message, String path) {
        return new FileOperationException(HttpStatus.BAD_REQUEST, code, message, null,
                Map.of("path", path == null ? "" : path));
    }

    private FileOperationException io(String operation, String path, IOException exception) {
        HttpStatus status;
        String code;
        if (exception instanceof NoSuchFileException) { status = HttpStatus.NOT_FOUND; code = "FILE_NOT_FOUND"; }
        else if (exception instanceof AccessDeniedException) { status = HttpStatus.FORBIDDEN; code = "ACCESS_DENIED"; }
        else if (exception instanceof FileSystemException) { status = HttpStatus.CONFLICT; code = "FILE_BUSY"; }
        else { status = HttpStatus.INTERNAL_SERVER_ERROR; code = "STORAGE_ERROR"; }
        return new FileOperationException(status, code, operation + "失败", exception,
                Map.of("path", path == null ? "" : path));
    }
}
