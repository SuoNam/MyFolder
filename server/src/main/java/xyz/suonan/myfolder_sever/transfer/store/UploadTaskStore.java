package xyz.suonan.myfolder_sever.transfer.store;

import xyz.suonan.myfolder_sever.transfer.model.UploadTask;

import java.util.Optional;

public interface UploadTaskStore {
    void save(UploadTask task);

    Optional<UploadTask> find(String uploadId);

    void delete(String uploadId);
}
