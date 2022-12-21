// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <QObject>

namespace QmlJSTools {
class QmlJSCodeStylePreferences;

/**
 * This class provides a central place for cpp tools settings.
 */
class QMLJSTOOLS_EXPORT QmlJSToolsSettings : public QObject
{
    Q_OBJECT

public:
    explicit QmlJSToolsSettings();
    ~QmlJSToolsSettings() override;

    static QmlJSCodeStylePreferences *globalCodeStyle();
};

} // namespace QmlJSTools
