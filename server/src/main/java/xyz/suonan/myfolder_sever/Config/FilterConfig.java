package xyz.suonan.myfolder_sever.Config;

import org.springframework.boot.web.servlet.FilterRegistrationBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import xyz.suonan.myfolder_sever.Filter.CorsFilter;
import xyz.suonan.myfolder_sever.Filter.TokenFilter;
@Configuration
public class FilterConfig  {
    @Bean
    public FilterRegistrationBean<CorsFilter> corsFilterRegistration(CorsFilter corsFilter) {
        FilterRegistrationBean<CorsFilter> registrationBean = new FilterRegistrationBean<>();
        registrationBean.setFilter(corsFilter);
        registrationBean.addUrlPatterns("/*");
        registrationBean.setOrder(1);
        return registrationBean;
    }

    @Bean
    public FilterRegistrationBean<TokenFilter> tokenFilterRegistration(TokenFilter tokenFilter) {
        FilterRegistrationBean<TokenFilter> registrationBean = new FilterRegistrationBean<>();
        registrationBean.setFilter(tokenFilter);
        registrationBean.addUrlPatterns("/file/uploadfile", "/file/uploadfile/*", "/file/createfolder", "/file/move", "/file/move-to-group", "/file/copy-to-group",
                "/file/delete", "/file/copy", "/directory/*",
                "/api/v1/transfers/*", "/api/v1/devices", "/api/v1/devices/*",
                "/api/v1/forwards", "/api/v1/forwards/*", "/api/v1/groups", "/api/v1/groups/*",
                "/user/me/storage", "/api/v1/admin/*");
        registrationBean.setOrder(2);
        return registrationBean;
    }
}
