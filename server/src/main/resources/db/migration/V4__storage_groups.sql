CREATE TABLE storage_group (
    group_id VARCHAR(36) PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    owner_account VARCHAR(30) NOT NULL,
    created_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    CONSTRAINT fk_storage_group_owner FOREIGN KEY (owner_account) REFERENCES user(account) ON DELETE CASCADE
);

CREATE TABLE storage_group_member (
    group_id VARCHAR(36) NOT NULL,
    user_account VARCHAR(30) NOT NULL,
    permission VARCHAR(10) NOT NULL,
    created_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (group_id, user_account),
    CONSTRAINT fk_storage_group_member_group FOREIGN KEY (group_id) REFERENCES storage_group(group_id) ON DELETE CASCADE,
    CONSTRAINT fk_storage_group_member_user FOREIGN KEY (user_account) REFERENCES user(account) ON DELETE CASCADE,
    CONSTRAINT chk_storage_group_permission CHECK (permission IN ('READ', 'WRITE', 'OWNER'))
);

CREATE INDEX idx_storage_group_member_user ON storage_group_member(user_account);
