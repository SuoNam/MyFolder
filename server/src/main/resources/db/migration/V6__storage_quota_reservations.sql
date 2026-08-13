CREATE TABLE storage_quota_reservation (
    upload_id VARCHAR(26) PRIMARY KEY,
    user_account VARCHAR(30) NOT NULL,
    reserved_bytes BIGINT NOT NULL,
    expires_at TIMESTAMP(3) NOT NULL,
    created_at TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    CONSTRAINT fk_quota_reservation_user FOREIGN KEY (user_account) REFERENCES user(account) ON DELETE CASCADE,
    INDEX idx_quota_reservation_user (user_account, expires_at)
);
