// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>

namespace HarmonyOs::Internal {

class HarmonyOsQtVersion : public QtSupport::QtVersion
{
public:
    HarmonyOsQtVersion();

    QString description() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;
    QSet<Utils::Id> availableFeatures() const override;
};

void setupHarmonyOsQtVersion();

} // namespace HarmonyOs::Internal
