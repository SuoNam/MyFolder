package xyz.suonan.myfolder_sever.Filter;

import com.fasterxml.jackson.databind.ObjectMapper;
import io.micrometer.common.lang.NonNullApi;
import jakarta.servlet.*;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;
import xyz.suonan.myfolder_sever.BaseMessage.BaseMessage;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import java.io.IOException;
@NonNullApi
@Component
public class TokenFilter extends OncePerRequestFilter {

    private final ObjectMapper objectMapper = new ObjectMapper();
    @Autowired
    private JwtGen jwtGen;

    @Override
    protected void doFilterInternal(HttpServletRequest request, HttpServletResponse response, FilterChain filterChain) throws ServletException, IOException {
        if ("OPTIONS".equalsIgnoreCase(request.getMethod())) {
            response.setStatus(HttpServletResponse.SC_OK);
            return;
        }
        String token = request.getHeader("Authorization");
        if(token == null || !jwtGen.verifyJwt(token)) {
            // 设置响应信息
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            response.setContentType("application/json;charset=UTF-8");

            // 将对象转换为 JSON 写入响应
            String json = objectMapper.writeValueAsString(new BaseMessage<>(200,"token验证失败",null));
            response.getWriter().write(json);
            return;
        }
        filterChain.doFilter(request, response);
    }

}
