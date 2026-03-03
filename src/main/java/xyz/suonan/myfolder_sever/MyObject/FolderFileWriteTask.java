package xyz.suonan.myfolder_sever.MyObject;

import lombok.AllArgsConstructor;
import lombok.Data;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkTaskCountService;
import xyz.suonan.myfolder_sever.Service.Writer.ChunkWriteHanler;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkBitmapService;

import java.io.InputStream;

@Data
@AllArgsConstructor
public class FolderFileWriteTask implements WriteTask {
    private String absolutePath;
    private String relativePath;
    private String uploadId;
    private int chunkIndex;
    private int totalChunks;
    private InputStream inputStream;
    private int Offset;
    private ChunkWriteHanler writer;
    private FileChunkBitmapService bitmapService;
    private FileChunkTaskCountService countService;

    @Override
    public void run() {
        try{
            writer.doWrite(this);
            bitmapService.setFileChunkBitmap(uploadId,relativePath,chunkIndex,true);
        }catch(Exception e){
            e.printStackTrace();
        }finally {
            //TODO::追加WriteTask计数器 计数器-1
            countService.removeFileChunkTaskCount(uploadId,absolutePath);
        }
    }
}
