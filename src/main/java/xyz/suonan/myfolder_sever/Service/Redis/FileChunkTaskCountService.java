package xyz.suonan.myfolder_sever.Service.Redis;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.Objects;

@Service
public class FileChunkTaskCountService {
    @Autowired
    private StringRedisTemplate redis;

    public void addFileChunkTaskCount(String uploadId,String fileName) {
        String safePath = URLEncoder.encode(fileName, StandardCharsets.UTF_8);
        String key="Folder:TaskCount"+uploadId+safePath;
        redis.opsForValue().increment(key);
    }

    public void removeFileChunkTaskCount(String uploadId,String fileName) {
        String safePath = URLEncoder.encode(fileName, StandardCharsets.UTF_8);
        String key="Folder:TaskCount"+uploadId+safePath;
        redis.opsForValue().decrement(key);
    }
    public int getFileChunkTaskCount(String uploadId,String fileName) {
        String safePath = URLEncoder.encode(fileName, StandardCharsets.UTF_8);
        String key="Folder:TaskCount"+uploadId+safePath;
        return Integer.parseInt(Objects.requireNonNull(redis.opsForValue().get(key)));
    }

}

