package xyz.suonan.myfolder_sever.Controller;

import org.springframework.context.annotation.Configuration;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;
import xyz.suonan.myfolder_sever.Handler.DeviceHandler;

@Configuration
@EnableWebSocket
public class WebSocketController implements WebSocketConfigurer {

    private final DeviceHandler deviceHandler;
    public WebSocketController(DeviceHandler deviceHandler) {
        this.deviceHandler = deviceHandler;
    }
    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
            registry.addHandler(deviceHandler,"/device").setAllowedOrigins("*");
    }
}
