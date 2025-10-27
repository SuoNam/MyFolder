package xyz.suonan.myfolder_sever.Service.Redis;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;

@Service
public class FileChunkSha256Service {

    @Autowired
    private StringRedisTemplate redis;

    public boolean setFileChunkSha256(String fileName, String chunkSha256) {
        String safePath = URLEncoder.encode(fileName, StandardCharsets.UTF_8);
        redis.opsForValue().set("Folder:file"+safePath, chunkSha256);
        return true;
    }

    public String getFileChunkSha256(String fileName) {
        String safePath = URLEncoder.encode(fileName, StandardCharsets.UTF_8);
        return redis.opsForValue().get("Folder:file"+safePath);
    }
}
