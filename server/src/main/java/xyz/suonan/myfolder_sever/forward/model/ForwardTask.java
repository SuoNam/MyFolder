package xyz.suonan.myfolder_sever.forward.model;

import lombok.Getter;
import lombok.Setter;

import java.time.Instant;
import java.util.List;

@Getter
@Setter
public class ForwardTask {
    private String forwardId;
    private String userId;
    private String sourceDeviceId;
    private String targetDeviceId;
    private String destinationPath;
    private boolean deleteSource;
    private ForwardChannel channel;
    /** High-entropy capability shared only with the two direct-transfer participants. */
    private String directTransferToken;
    private String relayUploadId;
    private List<ForwardFile> files;
    private List<String> directories;
    private long totalBytes;
    private long transferredBytes;
    private ForwardState state;
    private Instant createdAt;
    private Instant updatedAt;
    private String failureReason;
}
