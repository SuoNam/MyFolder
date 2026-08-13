package xyz.suonan.myfolder_sever.forward.model;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;

import java.io.IOException;
import java.util.Set;

public class ForwardFileDeserializer extends StdDeserializer<ForwardFile> {
    private static final Set<String> FIELDS = Set.of("path", "size", "sha256");

    public ForwardFileDeserializer() {
        super(ForwardFile.class);
    }

    @Override
    public ForwardFile deserialize(JsonParser parser, DeserializationContext context) throws IOException {
        JsonNode node = parser.getCodec().readTree(parser);
        if (!node.isObject() || !node.hasNonNull("path") || !node.has("size") || !node.hasNonNull("sha256")) {
            return (ForwardFile) context.handleUnexpectedToken(ForwardFile.class, parser);
        }
        var names = node.fieldNames();
        while (names.hasNext()) {
            String name = names.next();
            if (!FIELDS.contains(name)) {
                context.reportInputMismatch(ForwardFile.class, "Unknown ForwardFile field: %s", name);
            }
        }
        if (!node.get("size").canConvertToLong() || !node.get("size").isIntegralNumber()) {
            context.reportInputMismatch(ForwardFile.class, "ForwardFile size must be an integer");
        }
        return new ForwardFile(node.get("path").asText(), node.get("size").longValue(), node.get("sha256").asText());
    }
}
