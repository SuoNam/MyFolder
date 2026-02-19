//package xyz.suonan.myfolder_sever.Config;
//
//import org.springframework.context.annotation.Configuration;
//import org.springframework.web.servlet.config.annotation.CorsRegistry;
//import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;
//
//@Configuration
//public class CorsConfig implements WebMvcConfigurer {
//    @Override
//    public void addCorsMappings(CorsRegistry registry) {
//        registry.addMapping("/**")  // 允许所有路径
//                .allowedOriginPatterns("*")
//                .allowedMethods("GET", "POST", "PUT", "DELETE", "OPTIONS") // 允许的 HTTP 方法
//                .allowedHeaders("*") // 允许所有请求头
//                .allowCredentials(false) // 是否允许凭证（如 Cookie）
//                .maxAge(3600); // 预检请求缓存时间（秒）
//    }
//}
