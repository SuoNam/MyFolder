package xyz.suonan.myfolder_sever.Service.Writer;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.MyObject.FolderFileWriteTask;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkBitmapService;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.file.Paths;

@Service
public class FileChunkWriter implements ChunkWriteHanler {
    @Value("${basePath}")
    String basePath;
    @Autowired
    FileChunkBitmapService fileChunkBitmapService;
    @Override
    public void doWrite(FolderFileWriteTask fileWriteTask){
        File file=new File(String.valueOf(Paths.get(basePath,fileWriteTask.getPath())));
        file.getParentFile().mkdirs();

        try (RandomAccessFile raf = new RandomAccessFile(file, "rw")) {
            raf.seek(fileWriteTask.getOffset());
            raf.write(fileWriteTask.getInputStream().readAllBytes());
            //TODO::改为动态分片大小

        } catch (IOException e) {
            throw new RuntimeException("写入文件分片失败: " + e.getMessage(), e);
        }

    }
}
