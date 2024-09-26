// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "studioquickutils.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

StudioQuickUtils::StudioQuickUtils()
{
    m_locale = QLocale(QLocale::system());
    m_locale.setNumberOptions(QLocale::OmitGroupSeparator);
}

void StudioQuickUtils::registerDeclarativeType()
{
    qmlRegisterSingletonType<StudioQuickUtils>(
        "StudioQuickUtils", 1, 0, "Utils", [](QQmlEngine *, QJSEngine *) {
            return new StudioQuickUtils();
        });
}

const QLocale &StudioQuickUtils::locale() const
{
    return m_locale;
}

StudioQuickUtils::~StudioQuickUtils() {}

} // namespace QmlDesigner
