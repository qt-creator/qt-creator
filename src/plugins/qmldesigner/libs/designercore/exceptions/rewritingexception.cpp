// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewritingexception.h"

using namespace QmlDesigner;

RewritingException::RewritingException(int line,
                                       const QByteArray &function,
                                       const QByteArray &file,
                                       const QByteArray &description,
                                       const QString &documentTextContent)
    : Exception(line, function, file, QString::fromUtf8(description))
    , m_documentTextContent(documentTextContent)
{
    createWarning();
}

QString RewritingException::type() const
{
    return QLatin1String("RewritingException");
}

QString RewritingException::documentTextContent() const
{
    return m_documentTextContent;
}
