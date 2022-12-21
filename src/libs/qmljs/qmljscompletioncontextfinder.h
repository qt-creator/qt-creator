// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include <qmljs/qmljslineinfo.h>

#include <QStringList>
#include <QTextCursor>

namespace QmlJS {

class QMLJS_EXPORT CompletionContextFinder : public LineInfo
{
public:
    CompletionContextFinder(const QTextCursor &cursor);

    QStringList qmlObjectTypeName() const;
    bool isInQmlContext() const;

    bool isInLhsOfBinding() const;
    bool isInRhsOfBinding() const;

    bool isAfterOnInLhsOfBinding() const;
    QStringList bindingPropertyName() const;

    bool isInStringLiteral() const;
    bool isInImport() const;
    QString libVersionImport() const;

private:
    int findOpeningBrace(int startTokenIndex);
    void getQmlObjectTypeName(int startTokenIndex);
    void checkBinding();
    void checkImport();

    QTextCursor m_cursor;
    QStringList m_qmlObjectTypeName;
    QStringList m_bindingPropertyName;
    int m_startTokenIndex;
    int m_colonCount;
    bool m_behaviorBinding;
    bool m_inStringLiteral;
    bool m_inImport;
    QString m_libVersion;
};

} // namespace QmlJS
