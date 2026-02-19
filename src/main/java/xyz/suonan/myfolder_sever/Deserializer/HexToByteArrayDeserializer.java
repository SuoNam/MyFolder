package xyz.suonan.myfolder_sever.Deserializer;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import java.io.IOException;
import java.util.HexFormat;

public class HexToByteArrayDeserializer extends JsonDeserializer<byte[]> {
    @Override
    public byte[] deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        String hex = p.getText().trim();
        // 如果前端为空，返回空数组避免 NPE
        if (hex.isEmpty()) {
            return new byte[0];
        }
        return HexFormat.of().parseHex(hex);
    }
}