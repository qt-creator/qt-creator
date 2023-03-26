// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

namespace ProjectExplorer {

// Possibly used by "QtCreatorTerminalPlugin"
PROJECTEXPLORER_EXPORT Utils::FilePaths findFileInSession(const Utils::FilePath &filePath);

} // namespace ProjectExplorer
