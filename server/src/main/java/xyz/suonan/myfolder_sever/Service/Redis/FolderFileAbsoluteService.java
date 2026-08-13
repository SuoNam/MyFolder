package xyz.suonan.myfolder_sever.Service.Redis;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@Service
public class FolderFileAbsoluteService {
    @Autowired
    private StringRedisTemplate redis;

    public void createFolderFileAbsolute(String uploadId, String relativePath,String absolutePath){
        String key="MyFolder:"+uploadId;
        String relativeSafePath= URLEncoder.encode(relativePath, StandardCharsets.UTF_8);
        String absoluteSafePath= URLEncoder.encode(absolutePath, StandardCharsets.UTF_8);
        redis.opsForHash().put(key,relativeSafePath,absoluteSafePath);
    }

    public List<String> getFolderFileAbsolute(String uploadId){
        String key="MyFolder:"+uploadId;
        return redis.opsForHash().entries(uploadId).values().stream()
                .map(o -> URLDecoder.decode(o.toString(), StandardCharsets.UTF_8))
                .collect(Collectors.toList());
    }




}
