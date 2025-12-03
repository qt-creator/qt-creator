// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "studioquickutils.h"

#include <coreplugin/icore.h>
#include <QScreen>

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

QRect StudioQuickUtils::screenContaining(int x, int y) const
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screen->geometry().contains(x, y))
            return screen->geometry();
    }

    if (QScreen *primaryScreen = QGuiApplication::primaryScreen())
        return primaryScreen->geometry();
    return {};
}

StudioQuickUtils::~StudioQuickUtils() {}

} // namespace QmlDesigner
