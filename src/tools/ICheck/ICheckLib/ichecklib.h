#ifndef ICHECKLIB_H
#define ICHECKLIB_H

#include <QStringList>
#include "ichecklib_global.h"

namespace CPlusPlus{
    class ParseManager;
}

class ICHECKLIBSHARED_EXPORT ICheckLib {
public:
    ICheckLib();
    void ParseHeader(const QStringList& includePath, const QStringList& filelist);
    bool check(const ICheckLib& ichecklib /*ICheckLib from interface header*/);
    QStringList getErrorMsg();
private:
    CPlusPlus::ParseManager* pParseManager;
};

#endif // ICHECKLIB_H
