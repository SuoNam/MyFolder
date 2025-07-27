package xyz.suonan.myfolder_sever;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import xyz.suonan.myfolder_sever.Utils.FileZipService;

import java.io.IOException;
import java.nio.file.Path;


@SpringBootTest
public class Ziptest {
        @Autowired
        FileZipService fileZipService;

       @Test
        public void testGenZip() throws IOException {
            fileZipService.zipFolder(Path.of("C:\\Users\\Lenovo\\Desktop\\test"), Path.of("C:\\Users\\Lenovo\\Desktop\\test.zip"));
       }
}
