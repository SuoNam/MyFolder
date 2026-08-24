package xyz.suonan.myfolder_sever.transfer.store;

import xyz.suonan.myfolder_sever.transfer.model.UploadTask;

import java.util.Optional;
import java.util.List;

public interface UploadTaskStore {
    void save(UploadTask task);

    Optional<UploadTask> find(String uploadId);

    List<UploadTask> findAllByOwner(String ownerUserId);

    void delete(String uploadId);
}
