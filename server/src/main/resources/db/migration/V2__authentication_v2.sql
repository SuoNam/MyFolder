ALTER TABLE `user`
    MODIFY COLUMN `password` VARCHAR(100) NULL,
    ADD COLUMN `email` VARCHAR(320) NULL AFTER `password`,
    ADD COLUMN `email_verified_at` TIMESTAMP(3) NULL AFTER `email`,
    ADD COLUMN `created_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) AFTER `email_verified_at`,
    ADD COLUMN `updated_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) AFTER `created_at`;

CREATE UNIQUE INDEX `ux_user_email` ON `user` (`email`);

CREATE TABLE `auth_session` (
    `session_id` CHAR(36) NOT NULL,
    `user_account` VARCHAR(30) NOT NULL,
    `client_type` VARCHAR(16) NOT NULL,
    `refresh_token_hash` CHAR(64) NOT NULL,
    `created_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `last_used_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `expires_at` TIMESTAMP(3) NOT NULL,
    `revoked_at` TIMESTAMP(3) NULL,
    `user_agent` VARCHAR(255) NULL,
    `ip_address` VARCHAR(64) NULL,
    PRIMARY KEY (`session_id`),
    UNIQUE KEY `ux_auth_session_refresh` (`refresh_token_hash`),
    KEY `ix_auth_session_user` (`user_account`, `revoked_at`, `expires_at`),
    CONSTRAINT `fk_auth_session_user` FOREIGN KEY (`user_account`) REFERENCES `user` (`account`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `email_verification` (
    `verification_id` CHAR(36) NOT NULL,
    `email` VARCHAR(320) NOT NULL,
    `purpose` VARCHAR(32) NOT NULL,
    `code_hash` VARCHAR(100) NOT NULL,
    `created_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `expires_at` TIMESTAMP(3) NOT NULL,
    `consumed_at` TIMESTAMP(3) NULL,
    `attempts` INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`verification_id`),
    KEY `ix_email_verification_lookup` (`email`, `purpose`, `created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `oauth_identity` (
    `identity_id` CHAR(36) NOT NULL,
    `user_account` VARCHAR(30) NOT NULL,
    `provider` VARCHAR(16) NOT NULL,
    `provider_subject` VARCHAR(255) NOT NULL,
    `provider_email` VARCHAR(320) NULL,
    `provider_username` VARCHAR(255) NULL,
    `created_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `updated_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (`identity_id`),
    UNIQUE KEY `ux_oauth_provider_subject` (`provider`, `provider_subject`),
    UNIQUE KEY `ux_oauth_user_provider` (`user_account`, `provider`),
    CONSTRAINT `fk_oauth_identity_user` FOREIGN KEY (`user_account`) REFERENCES `user` (`account`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `oauth_transaction` (
    `state_hash` CHAR(64) NOT NULL,
    `provider` VARCHAR(16) NOT NULL,
    `client_type` VARCHAR(16) NOT NULL,
    `flow_type` VARCHAR(16) NOT NULL,
    `user_account` VARCHAR(30) NULL,
    `pkce_verifier` VARCHAR(128) NOT NULL,
    `return_uri` VARCHAR(512) NOT NULL,
    `created_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `expires_at` TIMESTAMP(3) NOT NULL,
    PRIMARY KEY (`state_hash`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `oauth_login_code` (
    `code_hash` CHAR(64) NOT NULL,
    `user_account` VARCHAR(30) NOT NULL,
    `client_type` VARCHAR(16) NOT NULL,
    `created_at` TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `expires_at` TIMESTAMP(3) NOT NULL,
    `consumed_at` TIMESTAMP(3) NULL,
    PRIMARY KEY (`code_hash`),
    CONSTRAINT `fk_oauth_login_code_user` FOREIGN KEY (`user_account`) REFERENCES `user` (`account`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
