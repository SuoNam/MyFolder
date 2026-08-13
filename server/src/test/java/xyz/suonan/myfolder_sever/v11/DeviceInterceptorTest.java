package xyz.suonan.myfolder_sever.v11;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.springframework.http.server.ServletServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.mock.web.MockHttpServletRequest;
import org.springframework.web.socket.WebSocketHandler;
import xyz.suonan.myfolder_sever.Interceptor.DeviceInterceptor;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import xyz.suonan.myfolder_sever.device.api.DeviceRegistrationRequest;
import xyz.suonan.myfolder_sever.device.service.DeviceService;

import java.nio.file.Path;
import java.util.HashMap;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

class DeviceInterceptorTest {
    @Test
    void websocketHandshakeRequiresMatchingJwtDeviceAndToken(@TempDir Path temp) throws Exception {
        ObjectMapper mapper = new ObjectMapper().registerModule(new JavaTimeModule());
        DeviceService devices = new DeviceService(mapper, temp.resolve("devices.json").toString());
        String token = devices.register("alice", new DeviceRegistrationRequest("laptop", "Laptop", "PC",
                "Linux", "1.1.1", "127.0.0.1", 49152)).deviceToken();
        JwtGen jwt = mock(JwtGen.class);
        when(jwt.subject("jwt")).thenReturn("alice");
        DeviceInterceptor interceptor = new DeviceInterceptor(jwt, devices);

        assertTrue(handshake(interceptor, token));
        assertFalse(handshake(interceptor, "wrong-token"));
    }

    private boolean handshake(DeviceInterceptor interceptor, String token) throws Exception {
        MockHttpServletRequest request = new MockHttpServletRequest("GET", "/device");
        request.addHeader("Authorization", "jwt");
        request.addHeader("X-Device-Token", token);
        request.setParameter("deviceId", "laptop");
        return interceptor.beforeHandshake(new ServletServerHttpRequest(request), mock(ServerHttpResponse.class),
                mock(WebSocketHandler.class), new HashMap<>());
    }
}
