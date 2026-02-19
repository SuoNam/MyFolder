package xyz.suonan.myfolder_sever.Error;

public enum ErrorType {
    DUPLICATE_KEY(-1,"sql","主键/外键冲突"),
    SYNTAX_ERROR(-2,"sql","SQL语法错误"),
    DATA_ERROR(-3,"sql","数据类型/长度错误"),
    DEADLOCK(-4,"sql","死锁/并发冲突"),
    UNKNOWN(0,"sql","未知错误");

    public int code;
    public String Type;
    public String Message;

    ErrorType(int i, String type, String s) {
        code = i;
        Type = type;
        Message = s;
    }

    public static ErrorType resolve(int code,String type){
        if (type == null||code==0) {
            return UNKNOWN;
        }
        for (ErrorType error : values()) {
            if (error.Type.equals(type)&&error.code == code) {
                return error;
            }
        }
        return UNKNOWN;
    }
}
