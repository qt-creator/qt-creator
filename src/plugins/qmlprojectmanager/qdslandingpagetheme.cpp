// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdslandingpagetheme.h"

#include "qmlprojectplugin.h"

#include <coreplugin/icore.h>

#include <QQmlEngine>
#include <QQmlComponent>

namespace QmlProjectManager {

QdsLandingPageTheme::QdsLandingPageTheme(Utils::Theme *originTheme, QObject *parent)
    : Utils::Theme(originTheme, parent)
{

}

void QdsLandingPageTheme::setupTheme(QQmlEngine *engine)
{
    Q_UNUSED(engine)

    static const int typeIndex = qmlRegisterSingletonType<QdsLandingPageTheme>(
        "LandingPageTheme", 1, 0, "Theme", [](QQmlEngine *, QJSEngine *) {
            return new QdsLandingPageTheme(Utils::creatorTheme(), nullptr);
        });
    QScopedPointer<QdsLandingPageTheme> theme(new QdsLandingPageTheme(Utils::creatorTheme(), nullptr));

    Q_UNUSED(typeIndex)
}

} //QmlProjectManager
