ALTER TABLE `user`
    ADD COLUMN `auto_accept_device_transfers` BOOLEAN NOT NULL DEFAULT FALSE
    AFTER `email_verified_at`;
