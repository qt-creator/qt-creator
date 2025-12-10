// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>

namespace ProjectExplorer { class Project; }

namespace CppEditor {

CPPEDITOR_EXPORT QString licenseTemplate(ProjectExplorer::Project *project,
                                         const Utils::FilePath &filePath = {},
                                         const QString &className = {});

CPPEDITOR_EXPORT bool usePragmaOnce(ProjectExplorer::Project *project);


} // CppEditor
