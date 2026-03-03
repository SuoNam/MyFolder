package xyz.suonan.myfolder_sever.Config;

import org.springframework.context.annotation.Configuration;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;
import xyz.suonan.myfolder_sever.Handler.DeviceHandler;
import xyz.suonan.myfolder_sever.Interceptor.DeviceInterceptor;

@Configuration
@EnableWebSocket
public class WebSocketConfig implements WebSocketConfigurer {

    private final DeviceHandler deviceHandler;
    public WebSocketConfig(DeviceHandler deviceHandler) {
        this.deviceHandler = deviceHandler;
    }
    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
            registry.addHandler(deviceHandler,"/device").addInterceptors(new DeviceInterceptor()).setAllowedOrigins("*");
    }
}
