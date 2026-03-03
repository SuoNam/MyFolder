package xyz.suonan.myfolder_sever.Service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.Dao.FileInfoDao;
import xyz.suonan.myfolder_sever.MyObject.FileBase;
import xyz.suonan.myfolder_sever.MyObject.FileInfo;

import java.util.List;

@Service
public class FileInfoService{
    @Autowired
    FileInfoDao fileInfoDao;

    public boolean sha256isExists(FileBase file){
        List<String> filelist=fileInfoDao.selectBySha256(file.getSha256());
        return filelist!=null&& !filelist.isEmpty();
    }

    public boolean insertFileInfo(FileInfo file){
        return fileInfoDao.insert(file)!=1;
    }
    public List<String> selectPathBySha256(FileInfo file){
        return fileInfoDao.selectBySha256(file.getSha256());
    }
    public boolean updateFileInfoByPath(String oldPath,String newFile){
        return fileInfoDao.updatePathByPath(oldPath,newFile)!=1;
    }
    public void deleteFileByPath(String path){
        fileInfoDao.deleteByPath(path);
    }
}
