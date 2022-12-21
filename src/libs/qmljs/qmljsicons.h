// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsast_p.h>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace QmlJS {

class IconsPrivate;

class QMLJS_EXPORT Icons
{
public:
    ~Icons();

    static Icons *instance();

    void setIconFilesPath(const QString &iconPath);

    QIcon icon(const QString &packageName, const QString typeName) const;
    static QIcon icon(AST::Node *node);

    static QIcon objectDefinitionIcon();
    static QIcon scriptBindingIcon();
    static QIcon publicMemberIcon();
    static QIcon functionDeclarationIcon();

private:
    Icons();
    static Icons *m_instance;
    IconsPrivate *d;
};

} // namespace QmlJS
