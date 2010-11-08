#ifndef PROFILEKEYWORDS_H
#define PROFILEKEYWORDS_H

#include <QtCore/QStringList>

namespace Qt4ProjectManager {

namespace Internal {

class ProFileKeywords
{
public:
    static QStringList variables();
    static QStringList functions();
    static bool isVariable(const QString &word);
    static bool isFunction(const QString &word);
private:
    ProFileKeywords();
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEKEYWORDS_H
