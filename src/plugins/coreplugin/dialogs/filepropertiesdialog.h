// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

namespace Utils { class FilePath; }

namespace Core {

CORE_EXPORT void executeFilePropertiesDialog(const Utils::FilePath &filePath);

} // Core
