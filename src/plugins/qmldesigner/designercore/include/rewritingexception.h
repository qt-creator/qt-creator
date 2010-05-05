#ifndef REWRITINGEXCEPTION_H
#define REWRITINGEXCEPTION_H

#include "exception.h"

namespace QmlDesigner {

class RewritingException: public Exception
{
public:
    RewritingException(int line,
                       const QString &function,
                       const QString &file, const QString &description);

    virtual QString type() const;
    virtual QString description() const;
private:
    QString m_description;
};

} // namespace QmlDesigner

#endif // REWRITINGEXCEPTION_H
