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
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
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
        Path filePath= Paths.get(basePath,filePathS);
        Resource resource = new UrlResource(filePath.toUri());

        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)  // 通用二进制流类型
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" + resource.getFilename() + "\"")
                .header("Access-Control-Expose-Headers", "Content-Disposition")
                .body(resource);
    }
    @PostMapping("/uploadfile")
    public BaseMessage<List<BaseMessage<Path>>> upLoadFile(@RequestParam("files") List<MultipartFile> files, @RequestParam("paths") List<String> paths) throws IOException {
        List<BaseMessage<Path>> fileBaseItemList=new ArrayList<>();
        for(int i=0;i<files.size();i++){
            Path start = Paths.get(basePath,paths.get(i));
            File dest = new File(start.toFile().getAbsolutePath());
            try {
                files.get(i).transferTo(dest); // 保存文件
                fileBaseItemList.add(new BaseMessage<Path>(200,"上传成功",start));
            } catch (IOException e) {
            //TODO::日志打印错误信息
                fileBaseItemList.add(new BaseMessage<>(500,"上传失败",start));
        }
        }
        return new BaseMessage<>(200,"上传完毕",fileBaseItemList);
    }
    //TODO::新建文件夹的接口
    @PostMapping("/createfolder")
    public BaseMessage<Object> createNewFile(@RequestBody Map<String,String> pathMap) throws IOException {
        Path start = Paths.get(basePath,pathMap.get("path"));
        try{
            Files.createDirectories(start);
        }catch (IOException e){
            return new BaseMessage<>(500,"创造失败",null);
        }
        return new BaseMessage<>(200,"创造成功",null);

    }

    //TODO::重命名接口
    @PostMapping("/rename")
    public BaseMessage<Object> rename(@RequestBody List<Map<String,String>> pathMap){
        List<BaseMessage<Path>> fileBaseItemList=new ArrayList<>();
        for (Map<String, String> stringStringMap : pathMap) {
            Path source = Paths.get(basePath, stringStringMap.get("targetPath"));
            Path target = Paths.get(basePath, stringStringMap.get("targetPath"));
            try {
                Files.move(source, target, StandardCopyOption.REPLACE_EXISTING);
                fileBaseItemList.add(new BaseMessage<Path>(200, "上传成功", source));
            } catch (IOException e) {
                fileBaseItemList.add(new BaseMessage<>(500, "上传失败", source));
            }
        }
        return new BaseMessage<>(200,"移动完毕",fileBaseItemList);
    }
    //TODO:删除接口
    @PostMapping("delete")
    public BaseMessage<Object> delete(@RequestBody List<Map<String,String>> pathMap){
        List<BaseMessage<Path>> fileBaseItemList=new ArrayList<>();
        for (Map<String, String> stringStringMap : pathMap) {
            Path source = Paths.get(basePath, stringStringMap.get("deletePath"));
            if (source.toFile().delete()) {
                fileBaseItemList.add(new BaseMessage<>(200, "删除成功", source));
            } else {
                fileBaseItemList.add(new BaseMessage<>(500, "删除失败", source));
            }
        }
        return new BaseMessage<>(200,"删除完毕",fileBaseItemList);
    }


}
