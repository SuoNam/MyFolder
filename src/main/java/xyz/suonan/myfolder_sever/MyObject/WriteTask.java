package xyz.suonan.myfolder_sever.MyObject;

import java.io.InputStream;

public interface WriteTask extends Runnable{
    String path = "";
    InputStream inputStream = null;
}
