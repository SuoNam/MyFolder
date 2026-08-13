package xyz.suonan.myfolder_sever.forward.model;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;

@JsonDeserialize(using = ForwardFileDeserializer.class)
public record ForwardFile(String path, long size, String sha256) {
}
