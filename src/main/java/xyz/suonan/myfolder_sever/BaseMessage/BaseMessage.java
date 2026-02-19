package xyz.suonan.myfolder_sever.BaseMessage;

public class BaseMessage<T> {
    public BaseMessage(int status, String message, T data) {
        this.status = status;
        this.message = message;
        this.data = data;
    }
    public int status;
    public String message;
    public T data;
}
