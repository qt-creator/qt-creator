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

#pragma once

#include "exception.h"
#include <QUrl>
#include <QCoreApplication>

namespace QmlJS {
class DiagnosticMessage;
}

namespace QmlDesigner {

class DocumentMessage {
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::DocumentMessage)
public:
    enum Type {
        NoError = 0,
        InternalError = 1,
        ParseError = 2
    };

public:
    DocumentMessage();
    DocumentMessage(const QmlJS::DiagnosticMessage &qmlError, const QUrl &document);
    DocumentMessage(const QString &shortDescription);
    DocumentMessage(Exception *exception);

    Type type() const
    { return m_type; }

    int line() const
    { return m_line; }

    int column() const
    { return m_column; }

    QString description() const
    { return m_description; }

    QUrl url() const
    { return m_url; }

    QString toString() const;

private:
    Type m_type;
    int m_line;
    int m_column;
    QString m_description;
    QUrl m_url;
};

} //QmlDesigner
