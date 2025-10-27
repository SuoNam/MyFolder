package xyz.suonan.myfolder_sever.Service.Checker;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.Service.FileInfoService;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkSha256Service;

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
        String fileSha256=calculateSHA256(file);
        //从redis中找到对应path的sha256
        String targetSha256=fileChunkSha256Service.getFileChunkSha256(path);
        return targetSha256.equals(fileSha256);
    }


    public String calculateSHA256(File file){
        try(InputStream in = new FileInputStream(file)){
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] buffer = new byte[8192];
            int len;
            while ((len = in.read(buffer)) != -1) {
                digest.update(buffer, 0, len);
            }
            byte[] hashBytes = digest.digest();
            return HexFormat.of().formatHex(hashBytes);
        } catch (Exception e) {
            throw new RuntimeException("计算 SHA-256 失败: " + e.getMessage(), e);
        }

    }
}
