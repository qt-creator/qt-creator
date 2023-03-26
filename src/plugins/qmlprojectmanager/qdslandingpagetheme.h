// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qqmlengine.h>

#include <utils/theme/theme.h>

namespace QmlProjectManager {

class QdsLandingPageTheme : public Utils::Theme
{
    Q_OBJECT
    QML_SINGLETON

public:
    static void setupTheme(QQmlEngine *engine);

private:
    QdsLandingPageTheme(Utils::Theme *originTheme, QObject *parent);
};

} //QmlProjectManager
