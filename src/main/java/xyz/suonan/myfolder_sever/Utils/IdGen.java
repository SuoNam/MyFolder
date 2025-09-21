package xyz.suonan.myfolder_sever.Utils;

import com.github.f4b6a3.uuid.UuidCreator;
import org.apache.commons.codec.binary.Base32;

import java.nio.ByteBuffer;
import java.util.UUID;

public final class IdGen {

    private IdGen(){}
    private static final Base32 base32 = new Base32();
    private static byte[] asBytes(UUID uuid) {
        ByteBuffer buffer = ByteBuffer.allocate(16);
        buffer.putLong(uuid.getMostSignificantBits());
        buffer.putLong(uuid.getLeastSignificantBits());
        return buffer.array();
    }
    public static String generateId() {
        UUID uuid_v7= UuidCreator.getTimeOrderedEpoch();
        byte[] uuidBytes = asBytes(uuid_v7);
        return base32.encodeAsString(uuidBytes).replace("=", "");
    }

}
