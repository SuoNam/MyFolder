package xyz.suonan.myfolder_sever.Config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Component;

@Component
public class SuperAdminBootstrap implements ApplicationRunner {
    private final JdbcTemplate jdbc; private final String accounts;
    public SuperAdminBootstrap(JdbcTemplate jdbc, @Value("${auth.super-admin-accounts:}") String accounts) { this.jdbc=jdbc; this.accounts=accounts; }
    @Override public void run(ApplicationArguments args) {
        for (String account : accounts.split(",")) if (!account.isBlank()) jdbc.update("UPDATE user SET system_role='SUPER_ADMIN' WHERE account=?", account.trim());
    }
}
