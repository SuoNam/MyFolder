#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QProcessEnvironment>
#include <QString>

namespace MyFolderServerConfig {

inline QString baseUrl()
{
    QString value = QProcessEnvironment::systemEnvironment().value(
        "MYFOLDER_BASE_URL", "https://api.myfolder.com.cn").trimmed();
    while (value.endsWith('/')) {
        value.chop(1);
    }
    return value;
}

}

#endif // SERVERCONFIG_H
