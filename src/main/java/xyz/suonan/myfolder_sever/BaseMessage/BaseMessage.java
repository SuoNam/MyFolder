package xyz.suonan.myfolder_sever.BaseMessage;

public class FileBaseMessage<T> {
    public FileBaseMessage(int status,String message, T data) {
        this.status = status;
        this.message = message;
        this.data = data;
    }
    public int status;
    public String message;
    public T data;
}
