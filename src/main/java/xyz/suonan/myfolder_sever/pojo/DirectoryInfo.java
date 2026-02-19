package xyz.suonan.myfolder_sever.pojo;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonRawValue;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.Setter;

import java.util.Date;
@Data
@NoArgsConstructor
@JsonIgnoreProperties(ignoreUnknown = true)
public class DirectoryInfo {
    public String id;
    public String directoryName;
    public int totalFiles;
    public int chunkSize;
    public long totalBytes;
    public String parentPath;
    public int status;
    public Date creationDate;

    public DirectoryInfo(String directoryName,int totalFiles,int chunkSize,long totalBytes,String parentPath) {
            this.directoryName = directoryName.trim();
            this.totalFiles = totalFiles;
            this.chunkSize = chunkSize;
            this.totalBytes = totalBytes;
            this.parentPath = parentPath;
            this.status = 0;
    }

}
