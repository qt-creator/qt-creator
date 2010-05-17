#ifndef REWRITINGEXCEPTION_H
#define REWRITINGEXCEPTION_H

#include "exception.h"

namespace QmlDesigner {

class RewritingException: public Exception
{
public:
    RewritingException(int line,
                       const QString &function,
                       const QString &file,
                       const QString &description,
                       const QString &documentTextContent);

    virtual QString type() const;
    virtual QString description() const;
    QString documentTextContent() const;
private:
    QString m_description;
    QString m_documentTextContent;
};

} // namespace QmlDesigner

#endif // REWRITINGEXCEPTION_H
