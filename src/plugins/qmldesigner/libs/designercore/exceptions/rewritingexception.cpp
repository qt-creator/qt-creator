// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewritingexception.h"

namespace QmlDesigner {

using namespace Qt::StringLiterals;

RewritingException::RewritingException(const QString &description,
                                       const QString &documentTextContent,
                                       const Sqlite::source_location &location)
    : Exception(location)
    , m_description{description}
    , m_documentTextContent(documentTextContent)
{
    createWarning();
}

const char *RewritingException::what() const noexcept
{
    return "RewritingException";
}

QString RewritingException::description() const
{
    return defaultDescription(location()) + ": " + m_description;
}

QString RewritingException::type() const
{
    return "RewritingException"_L1;
}

QString RewritingException::documentTextContent() const
{
    return m_documentTextContent;
}

} // namespace QmlDesigner
