package xyz.suonan.myfolder_sever.Utils;

import org.springframework.stereotype.Component;

import java.io.*;
import java.nio.file.*;
import java.util.zip.*;

@Component
public class FileZipService {

    public void zipFolder(Path sourceDirPath, Path zipFilePath) throws IOException {
        try (
                FileOutputStream fos = new FileOutputStream(zipFilePath.toFile());
                ZipOutputStream zos = new ZipOutputStream(fos)
        ) {

            Files.walk(sourceDirPath)
                    .filter(path -> !Files.isDirectory(path))
                    .forEach(path -> {
                        ZipEntry zipEntry = new ZipEntry(sourceDirPath.relativize(path).toString());
                        try {
                            zos.putNextEntry(zipEntry);
                            Files.copy(path, zos);
                            zos.closeEntry();
                        } catch (IOException e) {
                            throw new RuntimeException("压缩文件失败: " + path, e);
                        }
                    });
        }
    }

    public void unzip(Path zipFilePath, Path destDir) throws IOException {
        File dir = destDir.toFile();
        if (!dir.exists()) dir.mkdirs();

        try (
                FileInputStream fis = new FileInputStream(zipFilePath.toFile());
                ZipInputStream zis = new ZipInputStream(fis)
        ) {
            ZipEntry entry;
            while ((entry = zis.getNextEntry()) != null) {
                File newFile = Paths.get(destDir.toString(), entry.getName()).toFile();
                if (entry.isDirectory()) {
                    newFile.mkdirs();
                } else {
                    new File(newFile.getParent()).mkdirs();
                    try (FileOutputStream fos = new FileOutputStream(newFile)) {
                        byte[] buffer = new byte[1024];
                        int len;
                        while ((len = zis.read(buffer)) > 0) {
                            fos.write(buffer, 0, len);
                        }
                    }
                }
                zis.closeEntry();
            }
        }
    }
}
