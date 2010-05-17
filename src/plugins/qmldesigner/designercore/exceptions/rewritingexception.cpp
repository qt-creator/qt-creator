#include "rewritingexception.h"

using namespace QmlDesigner;

RewritingException::RewritingException(int line,
                                       const QString &function,
                                       const QString &file,
                                       const QString &description,
                                       const QString &documentTextContent):
        Exception(line, function, file), m_description(description), m_documentTextContent(documentTextContent)
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

QString RewritingException::documentTextContent() const
{
    return m_documentTextContent;
}
