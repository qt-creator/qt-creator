/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <documentmessage.h>

#include <qmljs/parser/qmljsengine_p.h>

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
        m_url(exception->file())
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
        str += tr("Error parsing");
    else if (m_type == InternalError)
        str += tr("Internal error");

    if (url().isValid()) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');

        str += QString("\"%1\"").arg(url().toString());
    }

    if (line() != -1) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');
        str += tr("line %1").arg(line());
    }

    if (column() != -1) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');

        str += tr("column %1").arg(column());
    }

    if (!str.isEmpty())
        QStringLiteral(": ");
    str += description();

    return str;
}
} //QmlDesigner
