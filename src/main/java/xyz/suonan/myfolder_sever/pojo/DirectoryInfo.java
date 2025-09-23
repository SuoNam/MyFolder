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
    public String manifestSummaryHash;
    @Setter(lombok.AccessLevel.NONE) // 防止 Lombok 生成 setOptions(String) 与我们自定义冲突
    @JsonRawValue                    // 序列化时按原始 JSON 输出（不再加引号）
    String options;
    public int status;
    public Date creationDate;

    public DirectoryInfo(String directoryName,int totalFiles,int chunkSize,long totalBytes,String parentPath,String manifestSummaryHash,String options) {
            this.directoryName = directoryName.trim();
            this.totalFiles = totalFiles;
            this.chunkSize = chunkSize;
            this.totalBytes = totalBytes;
            this.parentPath = parentPath;
            this.manifestSummaryHash = manifestSummaryHash;
            this.options = options;
            this.status = 0;
    }
    @JsonProperty("options")
    public void setOptions(Object options) throws JsonProcessingException {
        this.options = new ObjectMapper().writeValueAsString(options);
    }

}
