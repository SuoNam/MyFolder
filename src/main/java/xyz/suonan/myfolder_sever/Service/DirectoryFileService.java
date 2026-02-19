package xyz.suonan.myfolder_sever.Service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.Dao.DirectoryFileDao;

@Service
public class DirectoryFileService {
    @Autowired
    DirectoryFileDao directoryFileDao;

    public void addDirectoryFile(String uploadId,String relativePath){
        directoryFileDao.insertByIdAndRelativePath(uploadId,relativePath);
    }

    public void deleteDirectoryFile(String uploadId){
        directoryFileDao.deleteById(uploadId);
    }

}
