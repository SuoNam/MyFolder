package xyz.suonan.myfolder_sever.Interceptor;

import jakarta.servlet.http.HttpServletRequest;
import org.springframework.http.server.ServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.http.HttpStatus;
import org.springframework.http.server.ServletServerHttpRequest;
import org.springframework.web.socket.WebSocketHandler;
import org.springframework.web.socket.server.HandshakeInterceptor;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import xyz.suonan.myfolder_sever.device.service.DeviceService;

import java.util.Map;

public class DeviceInterceptor implements HandshakeInterceptor {
    private final JwtGen jwtGen;
    private final DeviceService deviceService;

    public DeviceInterceptor(JwtGen jwtGen, DeviceService deviceService) {
        this.jwtGen = jwtGen;
        this.deviceService = deviceService;
    }
    @Override
    public boolean beforeHandshake(ServerHttpRequest request, ServerHttpResponse response, WebSocketHandler wsHandler, Map<String, Object> attributes) throws Exception {
        if(request instanceof ServletServerHttpRequest){
            ServletServerHttpRequest servletRequest=(ServletServerHttpRequest)request;

            String authorization = servletRequest.getServletRequest().getHeader("X-MyFolder-Authorization");
            if (authorization == null || authorization.isBlank()) {
                authorization = servletRequest.getServletRequest().getHeader("Authorization");
            }
            String userId = authorization == null ? null : jwtGen.subject(authorization);
            String deviceId=servletRequest.getServletRequest().getParameter("deviceId");
            String deviceToken = servletRequest.getServletRequest().getHeader("X-Device-Token");
            if (deviceToken == null || deviceToken.isBlank()) {
                deviceToken = servletRequest.getServletRequest().getParameter("deviceToken");
            }
            if (userId == null || userId.isBlank() || deviceId == null || deviceId.isBlank()) {
                response.setStatusCode(HttpStatus.UNAUTHORIZED);
                return false;
            }
            try {
                deviceService.authenticate(userId, deviceId, deviceToken);
            } catch (DeviceService.DeviceException ex) {
                response.setStatusCode(HttpStatus.FORBIDDEN);
                return false;
            }
            HttpServletRequest httpRequest = servletRequest.getServletRequest();
            String ip = "";
            ip = httpRequest.getHeader("X-Forwarded-For");
            if (ip == null || ip.isEmpty() || "unknown".equalsIgnoreCase(ip)) {
                ip = httpRequest.getHeader("X-Real-IP");
            }
            if (ip == null || ip.isEmpty() || "unknown".equalsIgnoreCase(ip)) {
                ip = httpRequest.getRemoteAddr();
            }

            attributes.put("userId", userId);
            attributes.put("deviceId", deviceId);
            attributes.put("deviceAddress", ip);
        }

        return true;
    }

    @Override
    public void afterHandshake(ServerHttpRequest request, ServerHttpResponse response, WebSocketHandler wsHandler, Exception exception) {

    }
}
