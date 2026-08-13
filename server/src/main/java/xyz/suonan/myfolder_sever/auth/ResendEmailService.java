package xyz.suonan.myfolder_sever.auth;

import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestClient;
import org.springframework.web.client.RestClientResponseException;
import xyz.suonan.myfolder_sever.Config.AuthProperties;

import java.util.Map;

@Service
public class ResendEmailService {
    private final AuthProperties properties;
    private final RestClient client;

    public ResendEmailService(AuthProperties properties, RestClient.Builder builder) {
        this.properties = properties;
        this.client = builder.baseUrl("https://api.resend.com").build();
    }

    public void sendVerificationCode(String email, String code, AuthDtos.VerificationPurpose purpose,
                                     String idempotencyKey) {
        if (properties.getResend().getApiKey().isBlank()) {
            throw new AuthException(HttpStatus.SERVICE_UNAVAILABLE,
                    "EMAIL_SERVICE_NOT_CONFIGURED", "邮件验证码服务尚未配置");
        }

        String action = switch (purpose) {
            case REGISTER -> "注册 MyFolder";
            case RESET_PASSWORD -> "重置 MyFolder 密码";
            case SENSITIVE -> "确认敏感操作";
            case BIND_EMAIL -> "绑定邮箱";
        };
        String subject = action + " · 邮箱验证码";
        String text = "你的 MyFolder 验证码是：" + code
                + "\n\n该验证码 10 分钟内有效，请勿转发给任何人。"
                + "\n如果不是你本人操作，可以忽略这封邮件。";
        String html = verificationEmailHtml(action, code);

        try {
            client.post().uri("/emails")
                    .header("Authorization", "Bearer " + properties.getResend().getApiKey())
                    .header("Idempotency-Key", idempotencyKey)
                    .contentType(MediaType.APPLICATION_JSON)
                    .body(Map.of("from", properties.getResend().getFrom(), "to", new String[]{email},
                            "subject", subject, "text", text, "html", html))
                    .retrieve().toBodilessEntity();
        } catch (RestClientResponseException e) {
            throw new AuthException(HttpStatus.BAD_GATEWAY,
                    "EMAIL_DELIVERY_FAILED", "验证码邮件发送失败，请稍后重试");
        }
    }

    private static String verificationEmailHtml(String action, String code) {
        return """
                <!doctype html>
                <html lang="zh-CN">
                <head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"></head>
                <body style="margin:0;padding:0;background:#eef5f2;color:#0b2822;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Microsoft YaHei',Arial,sans-serif;">
                  <div style="display:none;max-height:0;overflow:hidden;opacity:0;">你的 MyFolder 验证码已生成，10 分钟内有效。</div>
                  <table role="presentation" width="100%%" cellspacing="0" cellpadding="0" border="0" style="background:#eef5f2;">
                    <tr><td align="center" style="padding:44px 16px;">
                      <table role="presentation" width="100%%" cellspacing="0" cellpadding="0" border="0" style="max-width:560px;background:#ffffff;border:1px solid #d9e7e1;border-radius:20px;box-shadow:0 14px 40px rgba(11,40,34,.08);overflow:hidden;">
                        <tr><td style="height:7px;background:linear-gradient(90deg,#1da779,#64dfc4);"></td></tr>
                        <tr><td style="padding:34px 38px 12px;">
                          <table role="presentation" cellspacing="0" cellpadding="0" border="0"><tr>
                            <td width="42" height="42" align="center" style="width:42px;height:42px;border-radius:12px;background:#1da779;color:#ffffff;font-size:23px;font-weight:800;line-height:42px;">↯</td>
                            <td style="padding-left:13px;">
                              <div style="font-size:20px;line-height:24px;font-weight:750;letter-spacing:-.3px;color:#082720;">MyFolder</div>
                              <div style="margin-top:2px;font-size:11px;line-height:16px;letter-spacing:1.8px;color:#789087;">SECURE ACCESS</div>
                            </td>
                          </tr></table>
                        </td></tr>
                        <tr><td style="padding:22px 38px 0;">
                          <div style="display:inline-block;padding:6px 10px;border-radius:999px;background:#e9f8f2;color:#147657;font-size:12px;font-weight:650;">%s</div>
                          <h1 style="margin:17px 0 8px;font-size:27px;line-height:36px;letter-spacing:-.5px;color:#082720;">验证你的操作</h1>
                          <p style="margin:0;color:#60746d;font-size:14px;line-height:23px;">请在 MyFolder 中输入下面的验证码：</p>
                        </td></tr>
                        <tr><td style="padding:22px 38px 4px;">
                          <div style="padding:22px 16px;border:1px solid #cce9de;border-radius:15px;background:#f1fbf7;text-align:center;">
                            <div style="font-family:Consolas,'SFMono-Regular','Courier New',monospace;font-size:34px;line-height:42px;font-weight:800;letter-spacing:10px;color:#0b4d3d;">%s</div>
                          </div>
                        </td></tr>
                        <tr><td style="padding:20px 38px 32px;">
                          <table role="presentation" width="100%%" cellspacing="0" cellpadding="0" border="0" style="border-top:1px solid #e8efec;">
                            <tr><td style="padding-top:19px;color:#687d75;font-size:13px;line-height:21px;">
                              <strong style="color:#304c43;">10 分钟内有效</strong><br>
                              MyFolder 不会向你索要此验证码。若非本人操作，请直接忽略本邮件。
                            </td></tr>
                          </table>
                        </td></tr>
                      </table>
                      <div style="padding:18px 12px 0;color:#8ba098;font-size:11px;line-height:18px;">此邮件由 MyFolder 安全服务自动发送，请勿直接回复。</div>
                    </td></tr>
                  </table>
                </body>
                </html>
                """.formatted(action, code);
    }
}
