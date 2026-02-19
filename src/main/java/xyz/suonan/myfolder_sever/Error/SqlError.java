package xyz.suonan.myfolder_sever.Error;
import java.sql.SQLDataException;
import java.sql.SQLIntegrityConstraintViolationException;
import java.sql.SQLSyntaxErrorException;
import java.sql.SQLTransactionRollbackException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class SqlError {
    private static Map<Class<? extends Throwable>, Integer> ERROR_CODE_MAP = new HashMap<>();
    static {
        Map<Class<? extends Throwable>, Integer> map = new HashMap<>();


        map.put(SQLIntegrityConstraintViolationException.class, -1); // 主键/外键/唯一约束冲突
        map.put(SQLSyntaxErrorException.class, -2);                  // SQL语法错误
        map.put(SQLDataException.class, -3);                         // 数据类型/长度错误
        map.put(SQLTransactionRollbackException.class, -4);          // 死锁/并发冲突

        map.put(org.springframework.dao.DuplicateKeyException.class, -1);             // 唯一约束冲突
        map.put(org.springframework.dao.DataIntegrityViolationException.class, -3);   // 非空/外键/数据类型不合法
        map.put(org.springframework.dao.ConcurrencyFailureException.class, -4);       // 并发冲突
//        map.put(org.springframework.dao.BadSqlGrammarException.class, -2);            // SQL语法错误

        ERROR_CODE_MAP = Collections.unmodifiableMap(map);
    }

    public static int resolve(Throwable cause) {
        if (cause == null) {
            return 0;
        }
        for (Map.Entry<Class<? extends Throwable>, Integer> entry : ERROR_CODE_MAP.entrySet()) {
            if (entry.getKey().isInstance(cause)) {
                return entry.getValue();
            }
        }
        return 0;
    }
}
