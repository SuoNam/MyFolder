package xyz.suonan.myfolder_sever.Service.Redis;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.RedisCallback;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.*;
import java.util.stream.IntStream;

@Service
public class FileChunkBitmapService {

    @Autowired
    private StringRedisTemplate redis;

    public boolean createFileChunkBitmap(String uploadId,String path,int totalChunks){
            try{
                String safePath = URLEncoder.encode(path, StandardCharsets.UTF_8);
                String key="MyFolder:"+uploadId+":"+safePath;
                redis.opsForValue().setBit(key,totalChunks-1,false);
                redis.expire(key, Duration.ofHours(24));
            }catch(Exception e){
                e.printStackTrace();
                return false;
            }

            return true;

    }

    public boolean deleteFileChunkBitmap(String uploadId,String path){
        try{
            String safePath = URLEncoder.encode(path, StandardCharsets.UTF_8);
            String key="MyFolder:"+uploadId+":"+safePath;
            redis.delete(key);
        }catch(Exception e){
            e.printStackTrace();
            return false;
        }

        return true;

    }

    public Map<String,Object> isComplete(String uploadId,String path,int totalChunks){
        Map<String,Object> result = new HashMap<>();
        String safePath = URLEncoder.encode(path, StandardCharsets.UTF_8);
        String key="MyFolder:"+uploadId+":"+safePath;
        byte[] keyBytes=key.getBytes(StandardCharsets.UTF_8);

        //检测切片是否存在
        if (Boolean.FALSE.equals(redis.hasKey(key))) {
            result.put("isComplete", false);
            result.put("missChunks", IntStream.range(0, totalChunks).boxed().toList());
            return result;
        }

        //所有切片都完成
        if(redis.execute((RedisCallback<Long>) connection->connection.bitCount(key.getBytes()))==totalChunks){
            result.put("isComplete",true);
            result.put("missChunks", Collections.emptyList());
            return result;
        };
        //存在切片不完成 检测哪些切片未完成
        List<Object> results =redis.executePipelined((RedisCallback<Object>) connection -> {
            for (int i = 0; i < totalChunks; i++) {
                connection.getBit(keyBytes, i);
            }
            return null;});

        List<Integer> missChunks = new ArrayList<>();
        for (int i = 0; i < results.size(); i++) {
            Boolean bit = (Boolean) results.get(i);
            if (!Boolean.TRUE.equals(bit)) {
                missChunks.add(i);
            }
        }

        result.put("isComplete",false);
        result.put("missChunks",missChunks);
        return result;

    }

    public boolean setFileChunkBitmap(String uploadId,String path,int chunkIndex,boolean isComplete){
        try{
            String safePath = URLEncoder.encode(path, StandardCharsets.UTF_8);
            String key="MyFolder:"+uploadId+":"+safePath;
            redis.opsForValue().setBit(key,chunkIndex,isComplete);
        }catch(Exception e){
            e.printStackTrace();
            return false;
        }
        return true;
    }


}
