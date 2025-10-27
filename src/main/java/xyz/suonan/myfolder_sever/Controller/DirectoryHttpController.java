package xyz.suonan.myfolder_sever.Controller;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
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
import xyz.suonan.myfolder_sever.MyObject.FileInfo;
import xyz.suonan.myfolder_sever.MyObject.FileInfoResponse;
import xyz.suonan.myfolder_sever.MyObject.FileInfoUpload;
import xyz.suonan.myfolder_sever.MyObject.FolderFileWriteTask;
import xyz.suonan.myfolder_sever.Service.*;
import xyz.suonan.myfolder_sever.Service.Checker.FileChunksCheck;
import xyz.suonan.myfolder_sever.Service.Executor.FileTaskExecutor;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkBitmapService;
import xyz.suonan.myfolder_sever.Service.Redis.FileChunkSha256Service;
import xyz.suonan.myfolder_sever.Service.Redis.FileCountService;
import xyz.suonan.myfolder_sever.Service.Writer.FileChunkWriter;
import xyz.suonan.myfolder_sever.Utils.FileZipService;
import xyz.suonan.myfolder_sever.Utils.IdGen;
import xyz.suonan.myfolder_sever.pojo.DirectoryInfo;

import java.io.*;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
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
    @Autowired
    FileChunkBitmapService fileChunkBitmapService;
    @Autowired
    FileChunkSha256Service fileChunkSha256Service;
    @Autowired
    FileChunkWriter fileChunkWriter;
    @Autowired
    FileTaskExecutor fileTaskExecutor;
    @Autowired
    FileChunksCheck fileChunksCheck;
    @Autowired
    FileCountService fileCountService;
    @Autowired
    ObjectMapper objectMapper;
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
        String uploadId=directoryInfo.getId();
        fileCountService.createFileCount(uploadId);
        Map<String,String> map = new HashMap<>();
        map.put("uploadId",uploadId);
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
        List<FileInfoUpload> files =filesInfo.get("entries");
        List<FileInfoResponse> data = new ArrayList<>();
        for (FileInfoUpload fileInfoUpload : files) {
            FileInfoResponse fileInfoResponse = new FileInfoResponse(fileInfoUpload.getPath(),fileInfoUpload.getSha256());
            //判断文件是否存在
            if(fileInfoService.sha256isExists(fileInfoUpload)){
                fileInfoResponse.setExists(true);
                //将目标文件复制到指定目录
                Path sourceFile =Paths.get(fileInfoService.selectPathBySha256(fileInfoUpload).get(0));
                Path targetFile=Paths.get(basePath,fileInfoUpload.getPath());
                try{
                    Files.createDirectories(targetFile.getParent());
                    Files.copy(sourceFile,targetFile, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.COPY_ATTRIBUTES);
                }catch (IOException e) {
                    throw new RuntimeException("复制文件失败: " + e.getMessage(), e);
                }
                //添加文件元信息到file_info
                fileInfoUpload.setPath(String.valueOf(Paths.get(basePath,fileInfoUpload.getPath())));
                fileInfoService.insertFileInfo(fileInfoUpload);
            }
            else{
                fileInfoResponse.setExists(false);
            }
            data.add(fileInfoResponse);
            //创建redis点位数组缓存  MyFolder:UploadId:X-File-Path
            fileChunkBitmapService.createFileChunkBitmap(uploadId,fileInfoUpload.getPath(),fileInfoUpload.getTotalChunks());
            //存entries中的文件信息到redis中
            String sha256Hex=HexFormat.of().formatHex(fileInfoUpload.getSha256());
            fileChunkSha256Service.setFileChunkSha256(String.valueOf(Paths.get(basePath,fileInfoUpload.getPath())),sha256Hex);
        }


        return new BaseMessage<>(200,"创建成功",data);
    }

    @PutMapping("/{uploadId}/chunks/{pageNumber}")
    public BaseMessage<Map<String,Map<String,String>>> uploadChunk(
            @PathVariable String uploadId,
            @PathVariable int pageNumber,
            HttpServletRequest request) throws IOException {
        //创建写任务 X-File-Path X-Total-Parts pageNumber Content-Range Content-sha256 content
        byte[] chunkData = request.getInputStream().readAllBytes();

        int Offset= Integer.parseInt(request.getHeader("Content-Range").split("-")[0]);
        String chunkSha256=request.getHeader("Content-sha256");
        Map<String,String> sha26Map = new HashMap<>();
        sha26Map.put("sha256",chunkSha256);
        InputStream inputStream = new ByteArrayInputStream(chunkData);
        String encodedPath = request.getHeader("X-File-Path");
        String filePath = URLDecoder.decode(encodedPath, StandardCharsets.UTF_8);
        FolderFileWriteTask fileWriteTask=new FolderFileWriteTask(filePath,uploadId,
                pageNumber,request.getIntHeader("X-Total-Parts"),inputStream,Offset,fileChunkWriter,fileChunkBitmapService);
        //加入线程池
        fileTaskExecutor.submit(fileWriteTask);

        Map<String,Map<String,String>> map = new HashMap<>();
        map.put("data",sha26Map);
        return new BaseMessage<>(200,"创建成功",map);
    }

    @PostMapping("/{uploadId}/chunks/complete")
    public BaseMessage<Map<String,String>> completeChunk(@PathVariable String uploadId,@RequestBody Map<String,String> completeChunk) throws JsonProcessingException {
        String path=completeChunk.get("filePath");
        int totalChunks=Integer.parseInt(completeChunk.get("totalChunks"));
        long fileSize=Long.parseLong(completeChunk.get("fileSize"));

        String completePath=String.valueOf(Paths.get(basePath,path));
        Map<String,String> map=new HashMap<>();
        //检测位点图是否完整
        Map<String,Object>bitmapCheckResult=fileChunkBitmapService.isComplete(uploadId,path,totalChunks);
        if(Boolean.FALSE.equals(bitmapCheckResult.get("isComplete"))){
            //获取哪些切片缺少
            List<Integer> missChunks= (List<Integer>) bitmapCheckResult.get("missChunks");
            //返回前端
            Map<String,List<Integer>> missChunksMap = new HashMap<>();
            missChunksMap.put("missChunks",missChunks);
            String missChunksJson=objectMapper.writeValueAsString(missChunksMap);
            map.put("data",missChunksJson);
            return new BaseMessage<>(500,"切片不完整",map);
        }

        //文件完整性检验
        if(!fileChunksCheck.check(completePath)){
            return new BaseMessage<>(200,"文件不完整",null);
        }
        //文件元信息存到数据库中
        File file = new File(completePath);
        fileInfoService.insertFileInfo(new FileInfo(completePath,fileSize,file.lastModified(), fileChunkSha256Service.getFileChunkSha256(path).getBytes()));
        fileCountService.addFileCount(uploadId);
        Map<String,String> filePathMap=new HashMap<>();
        filePathMap.put("filePath",path);
        String filePath=objectMapper.writeValueAsString(filePathMap);
        map.put("data",filePath);
        return new BaseMessage<>(200,"创建成功",map);
    }
    @PostMapping("/{uploadId}/complete")
    public BaseMessage<Map<String,String>> complete(@PathVariable String uploadId,@RequestBody Map<String,String> completeChunk) {
        if(fileCountService.getFileCount(uploadId)!=Integer.parseInt(completeChunk.get("totalFiles"))){
            return new BaseMessage<>(200,"未上传完全",null);
        }
        fileCountService.deleteFileCount(uploadId);
        return new BaseMessage<>(200,"上传成功",null);
    }

}

