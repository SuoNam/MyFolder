package xyz.suonan.myfolder_sever.Handler;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;
import xyz.suonan.myfolder_sever.device.service.DeviceService;

import java.time.Instant;
import java.util.LinkedHashMap;

@Component
@RequiredArgsConstructor
public class DeviceHandler extends TextWebSocketHandler {
    private final DeviceService deviceService;
    private final ObjectMapper objectMapper;

    @Override
    public void afterConnectionEstablished(WebSocketSession session) throws Exception {
        String userId = (String) session.getAttributes().get("userId");
        String deviceId = (String) session.getAttributes().get("deviceId");
        deviceService.attachSession(userId, deviceId, session);
        send(session, "hello.ack", deviceService.get(userId, deviceId), session.getId());
    }

    @Override
    public void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
        JsonNode root = objectMapper.readTree(message.getPayload());
        String action = root.path("action").asText();
        String userId = (String) session.getAttributes().get("userId");
        String deviceId = (String) session.getAttributes().get("deviceId");
        String msgId = root.path("msgId").asText();
        if ("heartbeat".equals(action)) {
            send(session, "heartbeat.ack", deviceService.heartbeat(userId, deviceId, null), msgId);
        } else if ("getDevices".equals(action) || "device.list".equals(action)) {
            send(session, "device.list", deviceService.list(userId), msgId);
        } else if ("hello".equals(action)) {
            send(session, "hello.ack", deviceService.get(userId, deviceId), msgId);
        }
    }

    @Override
    public void afterConnectionClosed(WebSocketSession session, CloseStatus status) {
        deviceService.detachSession((String) session.getAttributes().get("userId"),
                (String) session.getAttributes().get("deviceId"), session);
    }

    private void send(WebSocketSession session, String action, Object payload, String msgId) throws Exception {
        LinkedHashMap<String, Object> response = new LinkedHashMap<>();
        response.put("action", action);
        response.put("timestamp", Instant.now().toString());
        response.put("msgId", msgId);
        response.put("payload", payload);
        String serialized = objectMapper.writeValueAsString(response);
        synchronized (session) {
            if (session.isOpen()) {
                session.sendMessage(new TextMessage(serialized));
            }
        }
    }
}
