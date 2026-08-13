ALTER TABLE user
    ADD COLUMN storage_tier VARCHAR(12) NOT NULL DEFAULT 'FREE',
    ADD COLUMN system_role VARCHAR(20) NOT NULL DEFAULT 'USER';

CREATE TABLE storage_group_folder_acl (
    group_id VARCHAR(36) NOT NULL,
    folder_path VARCHAR(1024) NOT NULL,
    path_hash CHAR(64) NOT NULL,
    user_account VARCHAR(30) NOT NULL,
    permission VARCHAR(10) NOT NULL,
    created_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (group_id, path_hash, user_account),
    CONSTRAINT fk_folder_acl_group FOREIGN KEY (group_id) REFERENCES storage_group(group_id) ON DELETE CASCADE,
    CONSTRAINT fk_folder_acl_user FOREIGN KEY (user_account) REFERENCES user(account) ON DELETE CASCADE,
    CONSTRAINT chk_folder_acl_permission CHECK (permission IN ('NONE','READ','WRITE','MANAGE'))
);

CREATE TABLE storage_object (
    scope_type VARCHAR(10) NOT NULL,
    scope_id VARCHAR(80) NOT NULL,
    object_path VARCHAR(2048) NOT NULL,
    path_hash CHAR(64) NOT NULL,
    owner_account VARCHAR(30) NOT NULL,
    size_bytes BIGINT NOT NULL,
    updated_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (scope_type, scope_id, path_hash),
    CONSTRAINT fk_storage_object_owner FOREIGN KEY (owner_account) REFERENCES user(account) ON DELETE CASCADE,
    INDEX idx_storage_object_owner (owner_account)
);
