package xyz.suonan.myfolder_sever;

import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;
import org.springframework.boot.web.servlet.ServletComponentScan;


@SpringBootApplication()
@MapperScan("xyz.suonan.myfolder_sever.Dao")
@ServletComponentScan
public class MyFolderSeverApplication {
    public static void main(String[] args) {
        SpringApplication.run(MyFolderSeverApplication.class, args);
    }

}
