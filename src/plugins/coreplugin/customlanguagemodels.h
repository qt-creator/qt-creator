// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "core_global.h"

#include <utils/commandline.h>

namespace Core {

CORE_EXPORT QStringList availableLanguageModels();
CORE_EXPORT Utils::CommandLine commandLineForLanguageModel(const QString &model);

namespace Internal {

void setupCustomLanguageModels();

} // namespace Internal
} // namespace Core
