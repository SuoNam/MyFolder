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
import xyz.suonan.myfolder_sever.MyObject.FileBaseItem;
import xyz.suonan.myfolder_sever.Utils.FileZipService;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Objects;

@RequestMapping("/directory")
@RestController
public class DirectoryHttpController {
    @Autowired
    FileZipService fileZipService;

    @Value("basePath")
    private String basePath;


    @GetMapping("/downloaddirectory")
    public ResponseEntity<Resource> downLoadDirectory(String directoryPathS) throws IOException {
        //TODO::对文件夹压缩
        Path sourcePath = Paths.get(basePath,directoryPathS).normalize();
        Path targetPath = Paths.get(basePath,directoryPathS+".zip").normalize();

        if (!sourcePath.startsWith(basePath)) {
            throw new SecurityException("非法路径请求！");
        }

        fileZipService.zipFolder(sourcePath,targetPath);

        Resource resource = new UrlResource(targetPath.toUri());
        if (!resource.exists()) {
            throw new FileNotFoundException("压缩后的文件不存在: " + targetPath);
        }

        String encodedFileName = URLEncoder.encode(Objects.requireNonNull(resource.getFilename()), StandardCharsets.UTF_8);

        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)  // 通用二进制流类型
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"" + encodedFileName + "\"")
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

