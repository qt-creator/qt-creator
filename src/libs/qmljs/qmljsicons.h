// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljs_global.h>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QStringView)

namespace QmlJS::Icons {

namespace Internal {
class IconsStorage;
}

class QMLJS_EXPORT Provider
{
public:
    static Provider &instance();

    Provider(const Provider &) = delete;
    Provider &operator=(const Provider &) = delete;
    Provider(Provider &&) = delete;
    Provider &operator=(Provider &&) = delete;

    QIcon icon(QStringView typeName) const;

private:
    Provider();
    std::unique_ptr<const Internal::IconsStorage> m_stockPtr;
};

QIcon QMLJS_EXPORT objectDefinitionIcon();
QIcon QMLJS_EXPORT scriptBindingIcon();
QIcon QMLJS_EXPORT publicMemberIcon();
QIcon QMLJS_EXPORT functionDeclarationIcon();
QIcon QMLJS_EXPORT enumMemberIcon();
} // namespace QmlJS::Icons
