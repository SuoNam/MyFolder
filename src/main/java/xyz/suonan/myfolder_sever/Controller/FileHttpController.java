package xyz.suonan.myfolder_sever.Controller;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.io.Resource;
import org.springframework.core.io.UrlResource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Utils.WrapFileBaseItem;
import xyz.suonan.myfolder_sever.MyObject.Item.FileBaseItem;
import java.io.File;
import java.io.IOException;
import java.nio.file.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

@RequestMapping("/file")
@RestController
public class FileHttpController {
    private static final org.slf4j.Logger log =org.slf4j.LoggerFactory.getLogger(FileHttpController.class);
    @Value("${basePath}")
    private String basePath;
    @Autowired
    private WrapFileBaseItem wrapFileBaseItem;

    //获取文件夹内的内容
    /* 文件格式：{type:file,fileName:...,size:...,createDate:...}
    *  文件夹格式:{type:directory,directoryName:...,[文件||文件夹]}
    *  返回一个文件夹对象
    * */
    @PostMapping("/getfilelist")
    public BaseMessage<List<FileBaseItem>> getFileList(@RequestBody Map<String, String> directoryPathMap) throws IOException {
        String directoryPath=directoryPathMap.get("directoryPath");
        Path start = Paths.get(basePath,directoryPath);
        try (Stream<Path> stream = Files.walk(start, 1).filter(path -> !path.equals(start))) {
            List<FileBaseItem> fileBaseItemList=stream.map(wrapFileBaseItem::wrapFileBaseItem).toList();
            return new BaseMessage<>(200,"获取成功",fileBaseItemList);
        } catch (IOException e) {
            //日志打印错误信息
            log.error(e.getMessage());
            return new BaseMessage<>(500,"获取失败",null);
        }
    }
    @GetMapping("/downloadfile")
    public ResponseEntity<Resource> downLoadFile(@RequestParam String filePathS) throws IOException {
        Path filePath= Paths.get(basePath,filePathS);
        Resource resource = new UrlResource(filePath.toUri());

        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)  // 通用二进制流类型
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" + resource.getFilename() + "\"")
                .header("Access-Control-Expose-Headers", "Content-Disposition")
                .body(resource);
    }
    @PostMapping("/uploadfile")
    public BaseMessage<List<BaseMessage<String>>> upLoadFile(@RequestParam("files") List<MultipartFile> files, @RequestParam("path") String path) throws IOException {
        List<BaseMessage<String>> fileBaseItemList=new ArrayList<>();
        for (MultipartFile file : files) {
            Path path1 = Paths.get(basePath, path);
            if (!path1.toFile().exists() && !path1.toFile().isDirectory()) {
                fileBaseItemList.add(new BaseMessage<>(500, "上传失败:目录不存在", file.getOriginalFilename()));
                continue;
            }
            Path start = Paths.get(basePath, path, file.getOriginalFilename());
            File dest = new File(start.toFile().getAbsolutePath());
            try {
                file.transferTo(dest); // 保存文件
                //TODO::添加对file_info表的添加
                fileBaseItemList.add(new BaseMessage<>(200, "上传成功", file.getOriginalFilename()));
            } catch (IOException e) {
                //日志打印错误信息
                log.error(e.getMessage());
                fileBaseItemList.add(new BaseMessage<>(500, "上传失败：没有写入权限", file.getOriginalFilename()));
            }
        }
        return new BaseMessage<>(200,"上传完毕",fileBaseItemList);
    }
    @PostMapping("/createfolder")
    public BaseMessage<Object> createNewFile(@RequestBody Map<String,String> pathMap) throws IOException {
        Path start = Paths.get(basePath,pathMap.get("path"));
        try{
            Files.createDirectories(start);
        }catch (IOException e){
            log.error(e.getMessage());
            return new BaseMessage<>(500,"创造失败",null);
        }
        return new BaseMessage<>(200,"创造成功",null);

    }
    @PostMapping("/rename")
    public BaseMessage<List<BaseMessage<String>>> rename(@RequestBody List<Map<String,String>> pathMap){
        List<BaseMessage<String>> fileBaseItemList=new ArrayList<>();
        for (Map<String, String> stringStringMap : pathMap) {
            Path source = Paths.get(basePath, stringStringMap.get("targetPath"));
            Path target = Paths.get(basePath, stringStringMap.get("newPath"));
            try {
                Files.move(source, target, StandardCopyOption.REPLACE_EXISTING);
                //TODO::添加对file_info的修改
                fileBaseItemList.add(new BaseMessage<>(200, "移动成功", stringStringMap.get("targetPath")));
            } catch (IOException e) {
                log.error(e.getMessage());
                fileBaseItemList.add(new BaseMessage<>(500, "移动失败", stringStringMap.get("targetPath")));
            }
        }
        return new BaseMessage<>(200,"移动完毕",fileBaseItemList);
    }
    @PostMapping("delete")
    public BaseMessage<List<BaseMessage<String>>> delete(@RequestBody List<Map<String,String>> pathMap){
        List<BaseMessage<String>> fileBaseItemList=new ArrayList<>();
        for (Map<String, String> stringStringMap : pathMap) {
            Path source = Paths.get(basePath, stringStringMap.get("deletePath"));
            try{
                //TODO::修改为可删除文件夹
                //TODO::添加对file_info的删除
                Files.delete(source);
                fileBaseItemList.add(new BaseMessage<>(200, "删除成功", stringStringMap.get("deletePath")));
            }catch (NoSuchFileException e) {
                fileBaseItemList.add(new BaseMessage<>(500, "删除失败:文件或目录不存在", stringStringMap.get("deletePath")));
            } catch (IOException e) {
                fileBaseItemList.add(new BaseMessage<>(500, "删除失败:文件或目录权限不足|文件正在被另一个进程使用", stringStringMap.get("deletePath")));
            }
        }
        return new BaseMessage<>(200,"删除完毕",fileBaseItemList);
    }


}
