package xyz.suonan.myfolder_sever.Utils;

import java.nio.file.Path;

public final class SafeRelativePath {
    private SafeRelativePath() {
    }

    public static String normalize(String value) {
        if (value == null || value.isBlank() || value.indexOf('\0') >= 0 || value.contains("\\")
                || value.startsWith("/") || value.endsWith("/") || value.matches("^[A-Za-z]:.*")) {
            throw new IllegalArgumentException("Path must be a non-empty '/' separated relative path");
        }
        for (String segment : value.split("/", -1)) {
            if (segment.isBlank() || segment.equals(".") || segment.equals("..")) {
                throw new IllegalArgumentException("Path contains a forbidden segment");
            }
        }
        try {
            Path path = Path.of(value).normalize();
            String normalized = path.toString().replace('\\', '/');
            if (path.isAbsolute() || normalized.equals(".") || normalized.equals("..")
                    || normalized.startsWith("../")) {
                throw new IllegalArgumentException("Path escapes its root");
            }
            return normalized;
        } catch (RuntimeException exception) {
            throw new IllegalArgumentException("Invalid relative path", exception);
        }
    }
}
