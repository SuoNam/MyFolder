package xyz.suonan.myfolder_sever.v11;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.mock.web.MockFilterChain;
import org.springframework.mock.web.MockHttpServletRequest;
import org.springframework.mock.web.MockHttpServletResponse;
import xyz.suonan.myfolder_sever.Filter.TokenFilter;
import xyz.suonan.myfolder_sever.Utils.JwtGen;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

class TokenFilterTest {
    @Test
    void unauthorizedResponseUsesUnifiedEnvelope() throws Exception {
        ObjectMapper mapper = new ObjectMapper();
        JwtGen jwt = mock(JwtGen.class);
        TokenFilter filter = new TokenFilter(mapper, jwt);
        MockHttpServletRequest request = new MockHttpServletRequest("GET", "/api/v1/forwards");
        MockHttpServletResponse response = new MockHttpServletResponse();

        filter.doFilter(request, response, new MockFilterChain());

        JsonNode body = mapper.readTree(response.getContentAsByteArray());
        assertEquals(401, response.getStatus());
        assertEquals(401, body.path("status").asInt());
        assertEquals("UNAUTHORIZED", body.path("code").asText());
        assertEquals("/api/v1/forwards", body.path("path").asText());
        assertEquals(true, body.path("details").isObject());
    }

    @Test
    void validJwtPublishesAuthenticatedSubject() throws Exception {
        ObjectMapper mapper = new ObjectMapper();
        JwtGen jwt = mock(JwtGen.class);
        when(jwt.verifyJwt("jwt")).thenReturn(true);
        when(jwt.subject("jwt")).thenReturn("alice");
        TokenFilter filter = new TokenFilter(mapper, jwt);
        MockHttpServletRequest request = new MockHttpServletRequest("GET", "/api/v1/forwards");
        request.addHeader("Authorization", "jwt");
        MockHttpServletResponse response = new MockHttpServletResponse();

        filter.doFilter(request, response, new MockFilterChain());

        assertEquals("alice", request.getAttribute("myfolder.userId"));
        assertEquals(200, response.getStatus());
    }

    @Test
    void publicDirectoryDownloadDoesNotRequireJwt() throws Exception {
        TokenFilter filter = new TokenFilter(new ObjectMapper(), mock(JwtGen.class));
        MockHttpServletRequest request = new MockHttpServletRequest("GET", "/directory/downloaddirectory");
        MockHttpServletResponse response = new MockHttpServletResponse();

        filter.doFilter(request, response, new MockFilterChain());

        assertEquals(200, response.getStatus());
    }
}
