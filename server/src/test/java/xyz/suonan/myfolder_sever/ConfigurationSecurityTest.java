package xyz.suonan.myfolder_sever;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.Test;

import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.Properties;

import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class ConfigurationSecurityTest {
    @ParameterizedTest
    @ValueSource(strings = {
            "application.properties",
            "application-development.properties",
            "application-formal.properties"
    })
    void deploymentSpecificAndSensitiveValuesComeFromEnvironment(String resource) throws IOException {
        Properties properties = new Properties();
        try (InputStream input = getClass().getClassLoader().getResourceAsStream(resource)) {
            assertNotNull(input, resource + " must exist");
            properties.load(input);
        }

        List<String> protectedKeys = List.of(
                "jwt.secret",
                "spring.datasource.url",
                "spring.datasource.username",
                "spring.datasource.password",
                "spring.data.redis.host",
                "spring.data.redis.username",
                "spring.data.redis.password",
                "basePath",
                "server.ssl.key-store",
                "server.ssl.key-store-password"
        );
        for (String key : protectedKeys) {
            String value = properties.getProperty(key);
            if (value != null) {
                assertTrue(value.startsWith("${"), () -> resource + ": " + key
                        + " must use an environment placeholder");
            }
        }
    }

    @Test
    void formalProfileIsTlsByDefaultAndUsesModernProtocols() throws IOException {
        Properties properties = new Properties();
        try (InputStream input = getClass().getClassLoader()
                .getResourceAsStream("application-formal.properties")) {
            assertNotNull(input, "application-formal.properties must exist");
            properties.load(input);
        }

        assertEquals("${MYFOLDER_TLS_ENABLED:true}", properties.getProperty("server.ssl.enabled"));
        assertEquals("TLSv1.3,TLSv1.2", properties.getProperty("server.ssl.enabled-protocols"));
        assertTrue(properties.getProperty("server.ssl.key-store").startsWith("${MYFOLDER_TLS_KEY_STORE:"));
        assertTrue(properties.getProperty("server.ssl.key-store-password")
                .startsWith("${MYFOLDER_TLS_KEY_STORE_PASSWORD:"));
    }
}
