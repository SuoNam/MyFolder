package xyz.suonan.myfolder_sever.Controller;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.servlet.mvc.method.annotation.StreamingResponseBody;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Error.ErrorType;
import xyz.suonan.myfolder_sever.MyObject.FileInfoResponse;
import xyz.suonan.myfolder_sever.MyObject.FileInfoUpload;
import xyz.suonan.myfolder_sever.Service.DirectoryInfoService;
import xyz.suonan.myfolder_sever.Service.FileInfoService;
import xyz.suonan.myfolder_sever.Utils.FileZipService;
import xyz.suonan.myfolder_sever.Utils.IdGen;
import xyz.suonan.myfolder_sever.pojo.DirectoryInfo;

import java.io.*;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;

@RequestMapping("/directory")
@RestController
public class DirectoryHttpController {
    @Autowired
    FileZipService fileZipService;
    @Autowired
    DirectoryInfoService directoryInfoService;
    @Autowired
    FileInfoService fileInfoService;
    @Value("${basePath}")
    private String basePath;
    @GetMapping("/downloaddirectory")
    public ResponseEntity<StreamingResponseBody> downLoadDirectory(@RequestParam String directoryPathS) throws IOException {
        Path sourcePath = Paths.get(basePath, directoryPathS).normalize();
        Path targetPath = Paths.get(basePath, directoryPathS + ".zip").normalize();
        if (!sourcePath.startsWith(basePath)) {
            throw new SecurityException("非法路径请求！");
        }
        fileZipService.zipFolder(sourcePath, targetPath);
        File file = targetPath.toFile();
        if (!file.exists()) {
            throw new FileNotFoundException("压缩后的文件不存在: " + targetPath);
        }
        String encodedFileName = URLEncoder.encode(Objects.requireNonNull(file.getName()), StandardCharsets.UTF_8);
        StreamingResponseBody stream = outputStream -> {
            try (InputStream inputStream = new FileInputStream(file)) {
                inputStream.transferTo(outputStream);
            } finally {
                Files.deleteIfExists(targetPath);
            }
        };
        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)  // 通用二进制流类型
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" + encodedFileName + "\"")
                .body(stream);
    }
    @PostMapping("/directoryInfo")
    public BaseMessage<Object> upLoadDirectory(@RequestBody DirectoryInfo directoryInfo) {
        directoryInfo.setId(IdGen.generateId());
        directoryInfo.setCreationDate(new Date());

        int code=directoryInfoService.addDirectoryInfo(directoryInfo);
        if(code!=1){
            return new BaseMessage<>(500,"创建失败", ErrorType.resolve(code,"sql"));
        }
        Map<String,String> map = new HashMap<>();
        map.put("uploadId",directoryInfo.getId());
        map.put("maxConcurrency","3");
        map.put("dedup","true");
        return new BaseMessage<>(200,"创建成功",map);
    }
    @PostMapping("/filesInfo")
    public BaseMessage<List<FileInfoResponse>> filesInfo(@RequestParam  String uploadId,
                                                     @RequestBody Map<String, List<FileInfoUpload>> filesInfo) {
        if(!directoryInfoService.UuidIsExist(uploadId)){
            return new BaseMessage<>(200,"uuid不存在",null);
        }
        //TODO::遍历files数据库 若已经存在则标记其秒传并记录其秒传文件路径 若没有则标记其需要传输
        List<FileInfoUpload> files =filesInfo.get("entries");
        List<FileInfoResponse> data = new ArrayList<>();
        for (FileInfoUpload fileInfoUpload : files) {
            FileInfoResponse fileInfoResponse = new FileInfoResponse(fileInfoUpload.getPath(),fileInfoUpload.getSha256());
            if(!fileInfoService.sha256isExists(fileInfoUpload)){
                fileInfoResponse.setExists(false);
            }
            fileInfoResponse.setExists(false);
            data.add(fileInfoResponse);
        }

        //TODO::返回
        return new BaseMessage<>(200,"创建成功",data);
    }

    @PutMapping("/{uploadId}/chunks/{pageNumber}")
    public BaseMessage<Map<String,String>> uploadChunk(
            @PathVariable String uploadId,
            @PathVariable int pageNumber,
            HttpServletRequest request){
        return new BaseMessage<>(200,"创建成功",null);
    }

    @PostMapping("/{uploadId}/chunks/complete")
    public BaseMessage<Map<String,String>> completeChunk(@PathVariable String uploadId,@RequestBody Map<String,String> completeChunk) {
        return new BaseMessage<>(200,"创建成功",null);
    }
    @PostMapping("/{uploadId}/complete")
    public BaseMessage<Map<String,String>> complete(@PathVariable String uploadId,@RequestBody Map<String,String> completeChunk) {
        return new BaseMessage<>(200,"创建成功",null);
    }

}

