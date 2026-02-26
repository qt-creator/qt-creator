// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gntarget.h"

#include <utils/filepath.h>

namespace GNProjectManager::Internal::GNInfoParser {

struct Result
{
    GNTargetsList targets;
    Utils::FilePaths buildSystemFiles; // from build_settings/gen_input_files
    Utils::FilePath rootPath;          // from build_settings/root_path
    Utils::FilePath buildDir;          // from build_settings/build_dir
};

Result parse(const Utils::FilePath &projectJsonPath);

} // namespace GNProjectManager::Internal::GNInfoParser
