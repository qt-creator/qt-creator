// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>

#include <QCoreApplication>

namespace Core {

enum class InstallResult {
    Success,
    Error,
    NeedsRestart,
};

CORE_EXPORT InstallResult
executePluginInstallWizard(const Utils::FilePath &archive = {}, bool prepareForUpdate = false);

} // namespace Core
