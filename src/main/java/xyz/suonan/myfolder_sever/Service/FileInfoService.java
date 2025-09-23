package xyz.suonan.myfolder_sever.Service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.Dao.FileInfoDao;
import xyz.suonan.myfolder_sever.MyObject.FileBase;

import java.util.List;

@Service
public class FileInfoService{
    @Autowired
    FileInfoDao fileInfoDao;

    public boolean sha256isExists(FileBase file){
        List<String> filelist=fileInfoDao.select(file.getSha256());
        return filelist!=null&& !filelist.isEmpty();
    }
}
