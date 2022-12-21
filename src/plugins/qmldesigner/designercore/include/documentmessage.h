// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exception.h"

#include <qmldesignercorelib_global.h>

#include <QUrl>
#include <QCoreApplication>

namespace QmlJS {
class DiagnosticMessage;
}

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT DocumentMessage {
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
