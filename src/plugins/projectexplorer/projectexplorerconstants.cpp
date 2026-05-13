// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectexplorerconstants.h"

#include "projectexplorertr.h"

#include <QGuiApplication>

namespace ProjectExplorer::Constants {

QString msgAutoDetected()
{
    return Tr::tr("Automatically Managed");
}

QString msgAutoDetectedToolTip()
{
    return Tr::tr("Automatically managed by %1 or the installer.")
        .arg(QGuiApplication::applicationDisplayName());
}

QString msgManual()
{
    return Tr::tr("Manual");
}

} // namespace ProjectExplorer::Constants
