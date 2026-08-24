package xyz.suonan.myfolder_sever.forward.api;

public record ForwardSignalRequest(String kind, String type, String sdp,
                                   String candidate, String mid) {
}
