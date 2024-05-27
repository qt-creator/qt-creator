// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>

#include <QCoreApplication>

namespace Core {

CORE_EXPORT bool executePluginInstallWizard(const Utils::FilePath &archive = {});

} // namespace Core
