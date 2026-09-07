// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/result.h>

namespace Utils { class AspectContainer; }

namespace Core {

class CORE_EXPORT SettingsTransfer
{
public:
    Utils::AspectContainer *container = nullptr;
    Utils::Id type;
    QString displayName;
    QString fileNameBase;
};

// What an import found in a file: settings that exist in the container, and
// settings that do not, as happens with a file from a different version.
class CORE_EXPORT SettingsImport
{
public:
    int applied = 0;
    int ignored = 0;
};

CORE_EXPORT Utils::Result<> exportSettings(
    const SettingsTransfer &transfer, const Utils::FilePath &filePath);

CORE_EXPORT Utils::Result<SettingsImport> importSettings(
    const SettingsTransfer &transfer, const Utils::FilePath &filePath);

CORE_EXPORT Layouting::Layout settingsTransferButtons(const SettingsTransfer &transfer);

} // namespace Core
