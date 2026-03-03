package xyz.suonan.myfolder_sever.Controller;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.servlet.http.HttpServletRequest;
import lombok.RequiredArgsConstructor;
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
import xyz.suonan.myfolder_sever.Service.Redis.*;
import xyz.suonan.myfolder_sever.Service.Timer.PartTimer;
import xyz.suonan.myfolder_sever.Service.Timer.TotalTimer;
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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
@RequestMapping("/directory")
@RestController
@RequiredArgsConstructor
public class DirectoryHttpController {
    private final PartTimer partTimer;
    private final TotalTimer totalTimer;
    private final FileZipService fileZipService;
    private final DirectoryInfoService directoryInfoService;
    private final FileInfoService fileInfoService;
    private final FolderFileAbsoluteService folderFileAbsoluteService;
    private final FileChunkBitmapService fileChunkBitmapService;
    private final FileChunkSha256Service fileChunkSha256Service;
    private final FileChunkWriter fileChunkWriter;
    private final FileChunksCheck fileChunksCheck;
    private final FileChunkTaskCountService fileChunkTaskCountService;
    private final FileTaskExecutor fileTaskExecutor;
    private final FolderFilesStatusService folderFilesStatusService;
    private final UploadRecoveryManager uploadRecoveryManager;
    private final ObjectMapper objectMapper;
    @Value("${basePath}")
    private String basePath;
    private static final Logger log = LoggerFactory.getLogger(DirectoryHttpController.class);
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
        totalTimer.begin();
        partTimer.name="/directoryInfo";
        partTimer.begin();
        directoryInfo.setId(IdGen.generateId());
        directoryInfo.setCreationDate(new Date());
        int code=directoryInfoService.addDirectoryInfo(directoryInfo);
        if(code!=1){
            return new BaseMessage<>(500,"创建失败", ErrorType.resolve(code,"sql"));
        }
        String uploadId=directoryInfo.getId();
        //TODO::添加一个uploadId有效期
//        fileCountService.createFileCount(uploadId);
        Map<String,String> map = new HashMap<>();
        map.put("uploadId",uploadId);
        map.put("maxConcurrency","3");
        map.put("dedup","true");
        partTimer.close();
        return new BaseMessage<>(200,"创建成功",map);
    }
    @PostMapping("/filesInfo")
    public BaseMessage<List<FileInfoResponse>> filesInfo(@RequestParam  String uploadId,
                                                     @RequestBody Map<String, List<FileInfoUpload>> filesInfo) throws IOException {
        partTimer.name="/filesInfo";
        partTimer.begin();
        //TODO::添加一个验证uploadId
        if(!directoryInfoService.UuidIsExist(uploadId)){
            return new BaseMessage<>(200,"uuid不存在",null);
        }
        //获取upload的parentPath
        String parentName=directoryInfoService.getParentNameById(uploadId);
        List<FileInfoUpload> files =filesInfo.get("entries");
        List<FileInfoResponse> data = new ArrayList<>();
        for (FileInfoUpload fileInfoUpload : files) {
            String absolutePath = String.valueOf(Paths.get(basePath,parentName,fileInfoUpload.getPath()).normalize());
            if(!absolutePath.startsWith(Paths.get(basePath).normalize().toString())){
                return new BaseMessage<>(500,"路径错误",null);
            }
            String relativelyPath=Path.of(fileInfoUpload.getPath()).normalize().toString();
            //存上传文件夹 相对路径 绝对路径
            folderFileAbsoluteService.createFolderFileAbsolute(uploadId,relativelyPath,absolutePath);
            FileInfoResponse fileInfoResponse = new FileInfoResponse(fileInfoUpload.getPath(),fileInfoUpload.getSha256());
            //判断文件是不是空文件
            int totalChunks = fileInfoUpload.getTotalChunks();
            if(totalChunks==0){
                //处理空文件问题
                Path path=Path.of(absolutePath);
                Files.createDirectories(path.getParent());
                Files.createFile(Path.of(absolutePath));
                fileInfoResponse.setExists(true);
                fileInfoUpload.setPath(absolutePath);
                //添加文件元信息到file_info中 绝对路径
                fileInfoService.insertFileInfo(fileInfoUpload);
                folderFilesStatusService.setFileStatus(uploadId,relativelyPath,1);
            }else{
                //TODO::添加文件大小控制||对短期的重复上传没有文件大小限制
                if(fileInfoService.sha256isExists(fileInfoUpload)){
                    fileInfoResponse.setExists(true);
                    //将目标文件复制到指定目录
                    Path sourceFile =Paths.get(fileInfoService.selectPathBySha256(fileInfoUpload).get(0));
                    Path targetFile=Paths.get(absolutePath);
                    try{
                        Files.createDirectories(targetFile.getParent());
                        Files.copy(sourceFile,targetFile, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.COPY_ATTRIBUTES);
                        folderFilesStatusService.setFileStatus(uploadId,relativelyPath,1);
                    }catch (IOException e) {
                        //扫描directory_file 删除uploadId对应的项和其对应的文件
                        //TODO::验证uploadId过期后回滚
                        log.info("出现问题执行回滚");
                        uploadRecoveryManager.Recover(uploadId);
                        return new BaseMessage<>(500,"复制文件失败",null);
                        //throw new RuntimeException("复制文件失败: " + e.getMessage(), e);
                    }
                    fileInfoUpload.setPath(absolutePath);
                    //添加文件元信息到file_info中 绝对路径
                    fileInfoService.insertFileInfo(fileInfoUpload);
                    //记录完成文件数+1
                    //fileCountService.addFileCount(uploadId);
                }
                else{
                    fileInfoResponse.setExists(false);
                    //创建redis点位数组缓存  MyFolder:UploadId:X-File-Path 相对路径
                    fileChunkBitmapService.createFileChunkBitmap(uploadId,relativelyPath,totalChunks);
                    //存entries中的文件sha256到redis中 绝对路径
                    String sha256Hex=HexFormat.of().formatHex(fileInfoUpload.getSha256());
                    fileChunkSha256Service.setFileChunkSha256(absolutePath,sha256Hex);
                    //建立Folder:uploadId   filename values的Hash表 存文件写入状态 相对路径
                    folderFilesStatusService.createFileStatusBitmap(uploadId,relativelyPath);

                }
            }
            data.add(fileInfoResponse);

        }
        partTimer.close();
        return new BaseMessage<>(200,"创建成功",data);
    }
    @PutMapping("/{uploadId}/chunks/{pageNumber}")
    public BaseMessage<Map<String,Map<String,String>>> uploadChunk(
            @PathVariable String uploadId,
            @PathVariable int pageNumber,
            HttpServletRequest request) throws IOException {
        //TODO::添加一个验证uploadId
        partTimer.name="/"+uploadId+"/chunks/"+pageNumber;
        partTimer.begin();
        //创建写任务 X-File-Path X-Total-Parts pageNumber Content-Range Content-sha256 content
        byte[] chunkData = request.getInputStream().readAllBytes();
        int Offset= Integer.parseInt(request.getHeader("Content-Range").split("-")[0]);
        int totalChunks=request.getIntHeader("X-Total-Parts");
        String chunkSha256=request.getHeader("Content-sha256");
        Map<String,String> sha26Map = new HashMap<>();
        sha26Map.put("sha256",chunkSha256);
        InputStream inputStream = new ByteArrayInputStream(chunkData);
        String encodedPath = request.getHeader("X-File-Path");
        //相对路径
        String relativePath = URLDecoder.decode(encodedPath, StandardCharsets.UTF_8);
        String parentName=directoryInfoService.getParentNameById(uploadId);
        //绝对路径
        String absolutePath = String.valueOf(Paths.get(basePath,parentName,relativePath).normalize());
        if(!absolutePath.startsWith(Paths.get(basePath).normalize().toString())){
            return new BaseMessage<>(500,"路径错误",null);
        }
        FolderFileWriteTask fileWriteTask=new FolderFileWriteTask(absolutePath,relativePath,uploadId,
                pageNumber,totalChunks,inputStream,Offset,fileChunkWriter,fileChunkBitmapService,fileChunkTaskCountService);
        //加入线程池
        //WriteTask计数器 计数器+1
        fileChunkTaskCountService.addFileChunkTaskCount(uploadId,absolutePath);
        try{
            fileTaskExecutor.submit(fileWriteTask);
        }catch (Exception e) {
            // 提交失败，必须把刚才加的 1 减回去！
            fileChunkTaskCountService.removeFileChunkTaskCount(uploadId,absolutePath);
            throw e;
        }


        Map<String,Map<String,String>> map = new HashMap<>();
        map.put("data",sha26Map);
        partTimer.close();
        return new BaseMessage<>(200,"创建成功",map);
    }
    @PostMapping("/{uploadId}/chunks/complete")
    public BaseMessage<Map<String,String>> completeChunk(@PathVariable String uploadId,@RequestBody Map<String,String> completeChunk) throws IOException, InterruptedException {
        //TODO::添加一个验证uploadId
        partTimer.name="/"+uploadId+"/chunks/complete";
        partTimer.begin();
        //相对路径
        String relativePath=completeChunk.get("filePath");
        int totalChunks=Integer.parseInt(completeChunk.get("totalParts"));
        long fileSize=Long.parseLong(completeChunk.get("fileSize"));
        String parentName=directoryInfoService.getParentNameById(uploadId);
        String absolutePath=String.valueOf(Paths.get(basePath,parentName,relativePath).normalize());
        if(!absolutePath.startsWith(Paths.get(basePath).normalize().toString())){
            return new BaseMessage<>(500,"路径错误",null);
        }
        Map<String,String> map=new HashMap<>();
        //检测位点图是否完整
        //添加看是线程没写完导致的还是前端没有发过来 相对路径
        long startTime = System.currentTimeMillis();
        long maxWaitTime = 30 * 1000;
        //ToDO::权宜之计
        while (true) {
            Map<String, Object> bitmapCheckResult = fileChunkBitmapService.isComplete(uploadId, relativePath, totalChunks);
            boolean isComplete = (Boolean) bitmapCheckResult.get("isComplete");
            int currentTaskCount=fileChunkTaskCountService.getFileChunkTaskCount(uploadId,absolutePath);
            // 条件：所有线程结束 且 位图完整 -> 成功退出
            if (isComplete && currentTaskCount == 0) {
                break;
            }
            // 条件：线程都结束了 但 位图仍不完整 -> 真正的失败（回滚）
            if (!isComplete && currentTaskCount == 0) {
                //TODO::验证uploadId过期后回滚
                log.warn("检测到位图不完整，执行回滚 uploadId=" + uploadId);
                List<Integer> missChunks = (List<Integer>) bitmapCheckResult.get("missChunks");
                Map<String, List<Integer>> missChunksMap = Map.of("missChunks", missChunks);
                map.put("data", objectMapper.writeValueAsString(missChunksMap));
                uploadRecoveryManager.Recover(uploadId);
                return new BaseMessage<>(500, "切片不完整", map);
            }
            if (System.currentTimeMillis() - startTime > maxWaitTime) {
                log.error("等待分片写入超时 uploadId={}", uploadId);
                return new BaseMessage<>(504, "服务器处理超时，请重试", null);
            }
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return new BaseMessage<>(500, "系统中断", null);
            }
        }
        //文件完整性检验 绝对路径
        if(!fileChunksCheck.check(absolutePath)){
            //扫描directory_file 删除uploadId对应的项和其对应的文件
            uploadRecoveryManager.Recover(uploadId);
            return new BaseMessage<>(200,"文件不完整",null);
        }
        //Folder:uploadId   filename values的Hash表 存文件写入状态 相对路径
        folderFilesStatusService.setFileStatus(uploadId,relativePath,1);
        //添加文件元信息到file_info中 绝对路径
        File file = new File(absolutePath);
        fileInfoService.insertFileInfo(new FileInfo(absolutePath,fileSize,file.lastModified(),HexFormat.of().parseHex(fileChunkSha256Service.getFileChunkSha256(absolutePath))));

//        fileCountService.addFileCount(uploadId);
        Map<String,String> filePathMap=new HashMap<>();
        filePathMap.put("filePath",relativePath);
        String filePath=objectMapper.writeValueAsString(filePathMap);
        map.put("data",filePath);
        partTimer.close();
        return new BaseMessage<>(200,"创建成功",map);
    }
    @PostMapping("/{uploadId}/complete")
    public BaseMessage<Map<String,String>> complete(@PathVariable String uploadId,@RequestBody Map<String,String> completeChunk) throws InterruptedException, IOException {
        partTimer.name="/"+uploadId+"/complete";
        partTimer.begin();
        int fileCount=Integer.parseInt(completeChunk.get("totalFiles"));
        //查询当前的uploadId的文件状态数组 若都完成文件检验->放行  若未完成->阻塞等待
        int retry = 0, maxRetry = 5000;
        while (!folderFilesStatusService.getFileStatus(uploadId).values().stream().allMatch(v->v==1)) {
            if (retry++ > maxRetry) {
                //TODO::验证uploadId过期后回滚
                log.info("出现问题执行回滚");
                uploadRecoveryManager.Recover(uploadId);
                return new BaseMessage<>(500, "等待超时，仍有文件未完成", null);
            }
            Thread.sleep(200);
        }
        directoryInfoService.deleteDirectoryInfo(uploadId);
        partTimer.close();
        totalTimer.close();
        return new BaseMessage<>(200,"上传成功",null);
    }

}

