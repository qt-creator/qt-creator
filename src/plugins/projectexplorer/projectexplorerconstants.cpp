// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectexplorerconstants.h"

#include "projectexplorertr.h"

#include <coreplugin/icore.h>

namespace ProjectExplorer {
namespace Constants {

QString msgAutoDetected()
{
    return Tr::tr("Auto-detected");
}

QString msgAutoDetectedToolTip()
{
    return Tr::tr("Automatically managed by %1 or the installer.")
        .arg(Core::ICore::ideDisplayName());
}

QString msgManual()
{
    return Tr::tr("Manual");
}

} // namespace Constants
} // namespace ProjectExplorer
