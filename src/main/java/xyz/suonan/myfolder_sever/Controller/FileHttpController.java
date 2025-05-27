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
import xyz.suonan.myfolder_sever.MyObject.FileBaseItem;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

@RequestMapping("/file")
@RestController
public class FileHttpController {

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
            //TODO::日志打印错误信息
            return new BaseMessage<>(500,"获取失败",null);
        }
    }
    @GetMapping("/downloadfile")
    public ResponseEntity<Resource> downLoadFile(@RequestParam String filePathS) throws IOException {
        Path filePath= Paths.get(filePathS);
        Resource resource = new UrlResource(filePath.toUri());

        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)  // 通用二进制流类型
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" + resource.getFilename() + "\"")
                .body(resource);
    }
    @PostMapping("/uploadfile")
    public BaseMessage<FileBaseItem> upLoadFile(@RequestParam("file") MultipartFile file, @RequestParam("path") String path) throws IOException {
        File dest = new File(path);
        try {
            file.transferTo(dest); // 保存文件
            return new BaseMessage<>(200,"上传成功",null);
        } catch (IOException e) {
            //TODO::日志打印错误信息
            return new BaseMessage<>(500,"上传失败",null);
        }
    }

}
