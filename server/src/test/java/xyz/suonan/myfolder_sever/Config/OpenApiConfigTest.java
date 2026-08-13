package xyz.suonan.myfolder_sever.Config;

import io.swagger.v3.oas.models.OpenAPI;
import io.swagger.v3.oas.models.security.SecurityScheme;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

class OpenApiConfigTest {
    @Test
    void documentsRawJwtAuthorizationHeader() {
        OpenAPI api = new OpenApiConfig().myFolderOpenApi();
        SecurityScheme scheme = api.getComponents().getSecuritySchemes().get(OpenApiConfig.JWT_SCHEME);
        assertNotNull(scheme);
        assertEquals(SecurityScheme.Type.APIKEY, scheme.getType());
        assertEquals(SecurityScheme.In.HEADER, scheme.getIn());
        assertEquals("Authorization", scheme.getName());
    }
}
