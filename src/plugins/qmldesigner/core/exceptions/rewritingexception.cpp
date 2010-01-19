#include "rewritingexception.h"

using namespace QmlDesigner;

RewritingException::RewritingException(int line,
                                       const QString &function,
                                       const QString &file):
        Exception(line, function, file)
{
}

QString RewritingException::type() const
{
    return "RewritingException";
}
