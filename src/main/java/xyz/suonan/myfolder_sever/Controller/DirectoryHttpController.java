package xyz.suonan.myfolder_sever.Controller;

import org.springframework.core.io.Resource;
import org.springframework.core.io.UrlResource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.MyObject.FileBaseItem;

import java.io.File;
import java.io.IOException;
import java.net.MalformedURLException;
import java.nio.file.Path;
import java.nio.file.Paths;

@RequestMapping("/directory")
@RestController
public class DirectoryHttpController {


    @GetMapping("/downloaddirectory")
    public ResponseEntity<Resource> downLoadDirectory(String directoryPathS) throws MalformedURLException {
        Path directoryPath= Paths.get(directoryPathS);
        //TODO::对文件夹压缩
        Resource resource = new UrlResource(directoryPath.toUri());

        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)  // 通用二进制流类型
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" + resource.getFilename() + "\"")
                .body(resource);
    }
    @PostMapping("/uploaddirectory")
    public BaseMessage<FileBaseItem> upLoadDirectory(@RequestParam("file") MultipartFile file, @RequestParam("path") String path){
        File dest = new File(path);
        try {
            file.transferTo(dest); // 保存文件
            //TODO::对压缩包进行解压
            return new BaseMessage<>(200,"上传成功",null);
        } catch (IOException e) {
            //TODO::日志打印错误信息
            return new BaseMessage<>(500,"上传失败",null);
        }
    }
    }

