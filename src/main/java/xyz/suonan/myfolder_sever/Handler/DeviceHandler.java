package xyz.suonan.myfolder_sever.Handler;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;
import xyz.suonan.myfolder_sever.BaseMessage.WebSocketBaseMessage;
import xyz.suonan.myfolder_sever.MyObject.Device.Device;
import xyz.suonan.myfolder_sever.MyObject.Device.DeviceInfo;
import xyz.suonan.myfolder_sever.Service.Session.SessionRepository;

import java.util.List;
import java.util.Map;

@Component
@RequiredArgsConstructor
public class DeviceHandler extends TextWebSocketHandler {
    private final SessionRepository sessionRepository;
    private final ObjectMapper objectMapper;
    @Override
    public void afterConnectionEstablished(WebSocketSession session) throws Exception {
        String userId = (String) session.getAttributes().get("userId");
        String deviceId = (String) session.getAttributes().get("deviceId");
        String deviceName = (String) session.getAttributes().get("deviceName");
        String deviceAddress = (String) session.getAttributes().get("deviceAddress");
        Device device=new Device(userId,deviceId,deviceName,deviceAddress,session);
        sessionRepository.register(userId,device);
    }

    @Override
    public void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
        String payload = message.getPayload();
        JsonNode root = objectMapper.readTree(payload);
        String action = root.path("action").asText();
        //获取当前用户所有在线设备
        if(action.equals("getDevices")){
            WebSocketBaseMessage<Map<String, String>> msg=objectMapper.readValue(payload,
                    new TypeReference<>() {
                    }
            );
            List<DeviceInfo> deviceslist=sessionRepository.getDevices(msg.getPayload().get("userId"));
            session.sendMessage(new TextMessage(objectMapper.writeValueAsString(deviceslist)));
        }


    }
    @Override
    public void afterConnectionClosed(WebSocketSession session, CloseStatus status) throws Exception {
        String userId = (String) session.getAttributes().get("userId");
        String deviceId = (String) session.getAttributes().get("deviceId");
        sessionRepository.unregister(userId,deviceId);
    }

}
