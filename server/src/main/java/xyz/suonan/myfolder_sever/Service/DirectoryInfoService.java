package xyz.suonan.myfolder_sever.Service;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import xyz.suonan.myfolder_sever.Dao.DirectoryInfoDao;
import xyz.suonan.myfolder_sever.Error.SqlError;
import xyz.suonan.myfolder_sever.pojo.DirectoryInfo;

import java.util.List;

@Service
public class DirectoryInfoService {
    @Autowired
    DirectoryInfoDao directoryInfoDao;

    public int addDirectoryInfo(DirectoryInfo directoryInfo) {
        try{
            directoryInfoDao.insert(directoryInfo);
        } catch (Exception e){
            Throwable cause = e.getCause();
            return SqlError.resolve(cause);
        }
        return 1;
    }

    public boolean UuidIsExist(String uuid) {
        List<String> uuidList=directoryInfoDao.selectAll(uuid);
        return !uuidList.isEmpty();
    }


    public void deleteDirectoryInfo(String uuid) {
        directoryInfoDao.deleteByID(uuid);
    }

    public String getParentNameById(String uuid){
        List<String> parent_name_list=directoryInfoDao.selectParentPathById(uuid);

        if(parent_name_list.isEmpty()){
            return null;
        }
        return parent_name_list.get(0);
    }

    public String getDirectoryNameById(String uuid){
        List<String> directory_name_list=directoryInfoDao.selectParentPathById(uuid);

        if(directory_name_list.isEmpty()){
            return null;
        }
        return directory_name_list.get(0);

    }

}
