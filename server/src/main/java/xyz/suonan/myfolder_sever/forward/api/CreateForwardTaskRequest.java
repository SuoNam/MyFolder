package xyz.suonan.myfolder_sever.forward.api;

import xyz.suonan.myfolder_sever.forward.model.ForwardChannel;
import xyz.suonan.myfolder_sever.forward.model.ForwardFile;

import java.util.List;

public record CreateForwardTaskRequest(
        String sourceDeviceId,
        String targetDeviceId,
        String destinationPath,
        boolean deleteSource,
        ForwardChannel channel,
        String relayUploadId,
        List<ForwardFile> files) {
}
