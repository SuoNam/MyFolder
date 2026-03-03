package xyz.suonan.myfolder_sever.BaseMessage;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class WebSocketBaseMessage<T> {
    private String action;
    private long timestamp;
    private String msgId;
    private T payload;
}
