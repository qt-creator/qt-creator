// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <documentmessage.h>

#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/parser/qmljsdiagnosticmessage_p.h>

namespace QmlDesigner {

DocumentMessage::DocumentMessage():
        m_type(NoError),
        m_line(-1),
        m_column(-1)
{
}

DocumentMessage::DocumentMessage(Exception *exception):
        m_type(InternalError),
        m_line(exception->line()),
        m_column(-1),
        m_description(exception->description()),
        m_url(QUrl::fromLocalFile(exception->file()))
{
}

DocumentMessage::DocumentMessage(const QmlJS::DiagnosticMessage &qmlError, const QUrl &document):
        m_type(ParseError),
        m_line(qmlError.loc.startLine),
        m_column(qmlError.loc.startColumn),
        m_description(qmlError.message),
        m_url(document)
{
}

DocumentMessage::DocumentMessage(const QString &shortDescription) :
    m_type(ParseError),
    m_line(1),
    m_column(0),
    m_description(shortDescription),
    m_url()
{
}


QString DocumentMessage::toString() const
{
    QString str;

    if (m_type == ParseError)
        str += ::QmlDesigner::DocumentMessage::tr("Error parsing");
    else if (m_type == InternalError)
        str += ::QmlDesigner::DocumentMessage::tr("Internal error");

    if (url().isValid()) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');

        str += QString("\"%1\"").arg(url().toString());
    }

    if (line() != -1) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');
        str += ::QmlDesigner::DocumentMessage::tr("line %1\n").arg(line());
    }

    if (column() != -1) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');

        str += ::QmlDesigner::DocumentMessage::tr("column %1\n").arg(column());
    }

    if (!str.isEmpty())
        str += QStringLiteral(": ");
    str += description();

    return str;
}
} //QmlDesigner
