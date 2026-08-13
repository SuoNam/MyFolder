package xyz.suonan.myfolder_sever.Config;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.boot.web.servlet.FilterRegistrationBean;
import xyz.suonan.myfolder_sever.Filter.CorsFilter;
import xyz.suonan.myfolder_sever.Filter.TokenFilter;
import xyz.suonan.myfolder_sever.Utils.JwtGen;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;

class FilterConfigTest {
    @Test
    void allLegacyMutationRoutesRequireJwtWhilePublicReadsStayOpen() {
        FilterConfig config = new FilterConfig();
        TokenFilter tokenFilter = new TokenFilter(new ObjectMapper(), mock(JwtGen.class));
        FilterRegistrationBean<TokenFilter> registration = config.tokenFilterRegistration(tokenFilter);

        assertTrue(registration.getUrlPatterns().contains("/file/uploadfile"));
        assertTrue(registration.getUrlPatterns().contains("/file/uploadfile/*"));
        assertTrue(registration.getUrlPatterns().contains("/file/createfolder"));
        assertTrue(registration.getUrlPatterns().contains("/file/move"));
        assertTrue(registration.getUrlPatterns().contains("/file/delete"));
        assertTrue(registration.getUrlPatterns().contains("/file/copy"));
        assertFalse(registration.getUrlPatterns().contains("/file/getfilelist"));
        assertFalse(registration.getUrlPatterns().contains("/file/downloadfile"));
    }
}
