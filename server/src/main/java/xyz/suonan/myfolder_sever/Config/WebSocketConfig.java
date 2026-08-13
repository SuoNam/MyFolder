package xyz.suonan.myfolder_sever.Config;

import org.springframework.context.annotation.Configuration;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;
import xyz.suonan.myfolder_sever.Handler.DeviceHandler;
import xyz.suonan.myfolder_sever.Interceptor.DeviceInterceptor;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import xyz.suonan.myfolder_sever.device.service.DeviceService;

@Configuration
@EnableWebSocket
public class WebSocketConfig implements WebSocketConfigurer {

    private final DeviceHandler deviceHandler;
    private final JwtGen jwtGen;
    private final DeviceService deviceService;
    public WebSocketConfig(DeviceHandler deviceHandler, JwtGen jwtGen, DeviceService deviceService) {
        this.deviceHandler = deviceHandler;
        this.jwtGen = jwtGen;
        this.deviceService = deviceService;
    }
    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
            registry.addHandler(deviceHandler,"/device").addInterceptors(new DeviceInterceptor(jwtGen, deviceService)).setAllowedOrigins("*");
    }
}
