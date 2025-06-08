package xyz.suonan.myfolder_sever.Utils;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jws;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.security.Keys;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;
import xyz.suonan.myfolder_sever.Config.JwtProperties;

import java.nio.charset.StandardCharsets;
import java.security.Key;
import java.util.Date;
@Component
public class JwtGen {
    private JwtProperties jwtProperties;

    private Key key;

    @Autowired
    public JwtGen(JwtProperties jwtProperties) {
        this.jwtProperties = jwtProperties;
        this.key = Keys.hmacShaKeyFor(jwtProperties.getSecret().getBytes(StandardCharsets.UTF_8));
    }
    public String genJwt(String account){
        Date now = new Date();
        Date expiry = new Date(now.getTime()+jwtProperties.getExpiration().toMillis());
        return Jwts.builder()
                .setSubject(account)             // 主题，常用来存用户名或用户ID
                .setIssuer("AuthServer")        // 签发者
                .setIssuedAt(now)                 // 签发时间
                .setExpiration(expiry)            // 过期时间
                .claim("role", "admin")           // 添加自定义数据
                .signWith(key)             // 用密钥签名
                .compact();
    }

    public Boolean verifyJwt(String token){
        try{
            Jws<Claims> parsed = Jwts.parserBuilder()
                    .setSigningKey(key)
                    .build()
                    .parseClaimsJws(token);
        }catch (Exception e){
            return false;
        }
        return true;
    }

}
