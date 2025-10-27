package xyz.suonan.myfolder_sever.Service.Redis;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.util.Objects;

@Service
public class FileCountService {
    @Autowired
    private StringRedisTemplate redis;

    public boolean createFileCount(String uploadId){
        redis.opsForValue().set("MyFolder:"+uploadId+":"+"Count", "0");
        return true;
    }
    public boolean addFileCount(String uploadId) {
        String oldValue=redis.opsForValue().get("MyFolder:"+uploadId+":"+"Count");
        if (oldValue != null) {
            redis.opsForValue().set("MyFolder:"+uploadId+":"+"Count", String.valueOf(Integer.parseInt( oldValue)+1));
        }
        return true;
    }
    public boolean deleteFileCount(String uploadId) {
       redis.delete("MyFolder:"+uploadId+":"+"Count");
       return true;
    }

    public int getFileCount(String uploadId) {
        return Integer.parseInt(Objects.requireNonNull(redis.opsForValue().get("MyFolder:" + uploadId + ":" + "Count")));
    }

}
