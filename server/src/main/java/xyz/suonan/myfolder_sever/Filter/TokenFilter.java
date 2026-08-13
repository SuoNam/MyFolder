package xyz.suonan.myfolder_sever.Filter;

import com.fasterxml.jackson.databind.ObjectMapper;
import io.micrometer.common.lang.NonNullApi;
import jakarta.servlet.*;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;
import xyz.suonan.myfolder_sever.Utils.JwtGen;
import java.io.IOException;
import java.time.Instant;
import java.util.Map;
@NonNullApi
@Component
public class TokenFilter extends OncePerRequestFilter {

    private final ObjectMapper objectMapper;
    private final JwtGen jwtGen;

    public TokenFilter(ObjectMapper objectMapper, JwtGen jwtGen) {
        this.objectMapper = objectMapper;
        this.jwtGen = jwtGen;
    }

    @Override
    protected boolean shouldNotFilter(HttpServletRequest request) {
        return "OPTIONS".equalsIgnoreCase(request.getMethod())
                || ("GET".equalsIgnoreCase(request.getMethod())
                && "/directory/downloaddirectory".equals(request.getRequestURI()));
    }

    @Override
    protected void doFilterInternal(HttpServletRequest request, HttpServletResponse response, FilterChain filterChain) throws ServletException, IOException {
        String token = request.getHeader("Authorization");
        if(token == null || !jwtGen.verifyJwt(token)) {
            unauthorized(request, response);
            return;
        }
        String subject = jwtGen.subject(token);
        if (subject == null || subject.isBlank()) {
            unauthorized(request, response);
            return;
        }
        request.setAttribute("myfolder.userId", subject);
        filterChain.doFilter(request, response);
    }

    private void unauthorized(HttpServletRequest request, HttpServletResponse response) throws IOException {
        response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
        response.setContentType("application/json;charset=UTF-8");
        objectMapper.writeValue(response.getWriter(), Map.of(
                "timestamp", Instant.now().toString(),
                "status", HttpServletResponse.SC_UNAUTHORIZED,
                "code", "UNAUTHORIZED",
                "message", "Authentication is required",
                "path", request.getRequestURI(),
                "details", Map.of()));
    }

}
