package xyz.suonan.myfolder_sever.Config;

import io.swagger.v3.oas.models.Components;
import io.swagger.v3.oas.models.OpenAPI;
import io.swagger.v3.oas.models.info.Contact;
import io.swagger.v3.oas.models.info.Info;
import io.swagger.v3.oas.models.info.License;
import io.swagger.v3.oas.models.security.SecurityScheme;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class OpenApiConfig {
    public static final String JWT_SCHEME = "rawJwt";

    @Bean
    public OpenAPI myFolderOpenApi() {
        return new OpenAPI()
                .info(new Info()
                        .title("MyFolder Server API")
                        .description("MyFolder 1.1.1 HTTP API。Authorization 头直接传 JWT，不添加 Bearer 前缀。")
                        .version("1.1.1")
                        .contact(new Contact().name("MyFolder"))
                        .license(new License().name("Private project")))
                .components(new Components().addSecuritySchemes(JWT_SCHEME,
                        new SecurityScheme()
                                .type(SecurityScheme.Type.APIKEY)
                                .in(SecurityScheme.In.HEADER)
                                .name("Authorization")
                                .description("登录或注册返回的裸 JWT")));
    }
}
