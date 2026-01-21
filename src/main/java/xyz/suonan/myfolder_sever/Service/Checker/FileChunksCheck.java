package xyz.suonan.myfolder_sever.Service.Checker;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.Service.FileInfoService;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkSha256Service;
import xyz.suonan.myfolder_sever.Utils.FileHashUtil;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.HexFormat;

@Service
public class FileChunksCheck implements ChunksCheck{

    @Autowired
    FileChunkSha256Service fileChunkSha256Service;
    @Override
    public boolean check(String path) {
        File file = new File(path);
        String fileSha256=FileHashUtil.calculateSHA256(file);
        //从redis中找到对应path的sha256
        String targetSha256=fileChunkSha256Service.getFileChunkSha256(path);
        return targetSha256.equals(fileSha256);
    }

}
