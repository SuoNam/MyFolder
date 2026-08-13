package xyz.suonan.myfolder_sever.Controller;

import org.junit.jupiter.api.Test;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.setup.MockMvcBuilders;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import xyz.suonan.myfolder_sever.auth.*;

import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

class UserControllerTest {
    @Test
    void unknownAccountReturns401InsteadOf500() throws Exception {
        AccountService accounts = mock(AccountService.class);
        when(accounts.authenticate(anyString(), anyString())).thenThrow(
                new AuthException(HttpStatus.UNAUTHORIZED, "INVALID_CREDENTIALS", "账号或密码错误"));
        MockMvc mvc = mvc(accounts);

        mvc.perform(post("/user/login").contentType(MediaType.APPLICATION_JSON)
                        .content("{\"account\":\"missing\",\"password\":\"wrong-password\",\"clientType\":\"WEB\"}"))
                .andExpect(status().isUnauthorized())
                .andExpect(jsonPath("$.code").value("INVALID_CREDENTIALS"));
    }

    @Test
    void oauthProviderMustBeKnown() throws Exception {
        OAuthService oauth = mock(OAuthService.class);
        when(oauth.start(anyString(), org.mockito.ArgumentMatchers.any(), org.mockito.ArgumentMatchers.isNull()))
                .thenThrow(new AuthException(HttpStatus.BAD_REQUEST, "OAUTH_PROVIDER_INVALID", "不支持的登录提供方"));
        UserController controller = new UserController(mock(AccountService.class), mock(SessionService.class),
                mock(VerificationService.class), mock(AuthRepository.class), mock(JwtGen.class), oauth);
        MockMvc mvc = MockMvcBuilders.standaloneSetup(controller).setControllerAdvice(new AuthExceptionHandler()).build();

        mvc.perform(post("/user/oauth/other/start").contentType(MediaType.APPLICATION_JSON)
                        .content("{\"clientType\":\"WEB\",\"link\":false}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("OAUTH_PROVIDER_INVALID"));
    }

    private MockMvc mvc(AccountService accounts) {
        UserController controller = new UserController(accounts, mock(SessionService.class),
                mock(VerificationService.class), mock(AuthRepository.class), mock(JwtGen.class), mock(OAuthService.class));
        return MockMvcBuilders.standaloneSetup(controller).setControllerAdvice(new AuthExceptionHandler()).build();
    }
}
