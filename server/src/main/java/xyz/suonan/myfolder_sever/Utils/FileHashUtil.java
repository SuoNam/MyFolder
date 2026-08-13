package xyz.suonan.myfolder_sever.Utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.HexFormat;

public final class FileHashUtil {
    private FileHashUtil(){}
     public static String calculateSHA256(File file){
        try(InputStream in = new FileInputStream(file)){
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] buffer = new byte[8192];
            int len;
            while ((len = in.read(buffer)) != -1) {
                digest.update(buffer, 0, len);
            }
            byte[] hashBytes = digest.digest();
            return HexFormat.of().formatHex(hashBytes);
        } catch (Exception e) {
            throw new RuntimeException("计算 SHA-256 失败: " + e.getMessage(), e);
        }

    }
}
