package xyz.suonan.myfolder_sever.Service.Redis;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@Service
public class FolderFilesStatusService {

    @Autowired
    private StringRedisTemplate redis;

    //开辟文件夹文件状态位图
    public void createFileStatusBitmap(String uploadId, String path){
        String key="Folder:"+uploadId;
        redis.opsForHash().put(key,path,"0");
    }

    //将对应文件状态设置为1
    public void setFileStatus(String uploadId,String path,int fileStatus){
        String key="Folder:"+uploadId;
        redis.opsForHash().put(key,path,String.valueOf(fileStatus));
    }

    //获取文件夹所有文件状态列表
    public Map<String,Integer> getFileStatus(String uploadId){
        String key="Folder:"+uploadId;

        return redis.opsForHash().entries(key).entrySet().stream().collect(Collectors.toMap(
                e -> e.getKey().toString(),
                e -> Integer.parseInt(e.getValue().toString())));

    }

}
