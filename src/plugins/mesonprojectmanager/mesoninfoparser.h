// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildoptions.h"
#include "target.h"

#include <utils/filepath.h>

namespace MesonProjectManager::Internal::MesonInfoParser {

struct Result
{
    TargetsList targets;
    BuildOptionsList buildOptions;
    Utils::FilePaths buildSystemFiles;
};

Result parse(const Utils::FilePath &buildDir);
Result parse(const QByteArray &data);
Result parse(QIODevice *introFile);

} // namespace MesonProjectManager::Internal::MesonInfoParser
