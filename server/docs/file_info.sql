CREATE TABLE IF NOT EXISTS `file_info` (
    `path` VARCHAR(512) NOT NULL,
    `sha256` BINARY(32) NOT NULL,
    `size` BIGINT UNSIGNED NOT NULL,
    `mtime` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`path`),
    KEY `file_info_sha256_idx` (`sha256`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
