package xyz.suonan.myfolder_sever.MyObject;

import lombok.AllArgsConstructor;
import lombok.Data;
import xyz.suonan.myfolder_sever.Service.Writer.ChunkWriteHanler;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkBitmapService;

import java.io.InputStream;

@Data
@AllArgsConstructor
public class FolderFileWriteTask implements WriteTask {
    private String path;
    private String uploadId;
    private int chunkIndex;
    private int totalChunks;
    private InputStream inputStream;
    private int Offset;
    private ChunkWriteHanler writer;
    private FileChunkBitmapService bitmapService;
    @Override
    public void run() {
        try{
            writer.doWrite(this);
        }catch(Exception e){
            e.printStackTrace();
        }
        bitmapService.setFileChunkBitmap(uploadId,path,chunkIndex-1,true);
    }
}
