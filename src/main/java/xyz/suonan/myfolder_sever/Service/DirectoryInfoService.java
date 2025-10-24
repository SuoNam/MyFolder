package xyz.suonan.myfolder_sever.Service;
import org.apache.ibatis.exceptions.PersistenceException;
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


}
