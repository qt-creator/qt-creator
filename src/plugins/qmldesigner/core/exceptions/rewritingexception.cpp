#include "rewritingexception.h"

using namespace QmlDesigner;

RewritingException::RewritingException(int line,
                                       const QString &function,
                                       const QString &file, const QString &description):
        Exception(line, function, file), m_description(description)
{
}

QString RewritingException::type() const
{
    return "RewritingException";
}

QString RewritingException::description() const
{
    return m_description;
}
