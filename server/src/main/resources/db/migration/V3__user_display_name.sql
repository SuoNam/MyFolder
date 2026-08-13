ALTER TABLE `user`
    ADD COLUMN `display_name` VARCHAR(40) NULL AFTER `account`;

UPDATE `user`
SET `display_name` = `account`
WHERE `display_name` IS NULL OR TRIM(`display_name`) = '';

ALTER TABLE `user`
    MODIFY COLUMN `display_name` VARCHAR(40) NOT NULL;
