#ifndef REWRITINGEXCEPTION_H
#define REWRITINGEXCEPTION_H

#include "exception.h"

namespace QmlDesigner {

class RewritingException: public Exception
{
public:
    RewritingException(int line,
                       const QString &function,
                       const QString &file);

    virtual QString type() const;
};

} // namespace QmlDesigner

#endif // REWRITINGEXCEPTION_H
