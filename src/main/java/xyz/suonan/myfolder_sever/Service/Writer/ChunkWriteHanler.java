package xyz.suonan.myfolder_sever.Service.Writer;

import xyz.suonan.myfolder_sever.MyObject.FolderFileWriteTask;

public interface ChunkWriteHanler {
    void doWrite(FolderFileWriteTask task);
}
