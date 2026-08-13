CREATE TABLE IF NOT EXISTS `user` (
    `account` VARCHAR(30) NOT NULL,
    `password` VARCHAR(100) NULL,
    PRIMARY KEY (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
