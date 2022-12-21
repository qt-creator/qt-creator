// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectexplorerconstants.h"

#include <coreplugin/icore.h>

#include <QCoreApplication>
#include <QString>

namespace ProjectExplorer {
namespace Constants {

QString msgAutoDetected()
{
    return QCoreApplication::translate("ProjectExplorer", "Auto-detected");
}

QString msgAutoDetectedToolTip()
{
    return QCoreApplication::translate("ProjectExplorer",
                                       "Automatically managed by %1 or the installer.")
        .arg(Core::ICore::ideDisplayName());
}

QString msgManual()
{
    return QCoreApplication::translate("ProjectExplorer", "Manual");
}

} // namespace Constants
} // namespace ProjectExplorer
