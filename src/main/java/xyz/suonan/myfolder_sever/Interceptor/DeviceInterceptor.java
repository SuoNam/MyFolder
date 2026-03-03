package xyz.suonan.myfolder_sever.Interceptor;

import jakarta.servlet.http.HttpServletRequest;
import org.springframework.http.server.ServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.http.server.ServletServerHttpRequest;
import org.springframework.web.socket.WebSocketHandler;
import org.springframework.web.socket.server.HandshakeInterceptor;

import java.util.Map;

public class DeviceInterceptor implements HandshakeInterceptor {
    @Override
    public boolean beforeHandshake(ServerHttpRequest request, ServerHttpResponse response, WebSocketHandler wsHandler, Map<String, Object> attributes) throws Exception {
        if(request instanceof ServletServerHttpRequest){
            ServletServerHttpRequest servletRequest=(ServletServerHttpRequest)request;
            String userId=servletRequest.getServletRequest().getParameter("userId");
            String deviceId=servletRequest.getServletRequest().getParameter("deviceId");
            String deviceName=servletRequest.getServletRequest().getParameter("deviceName");

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
            attributes.put("deviceName", deviceName);
            attributes.put("deviceAddress", ip);
        }

        return true;
    }

    @Override
    public void afterHandshake(ServerHttpRequest request, ServerHttpResponse response, WebSocketHandler wsHandler, Exception exception) {

    }
}
