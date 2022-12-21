// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace Python::Internal {

enum class ReplType { Unmodified, Import, ImportToplevel };
void openPythonRepl(QObject *parent, const Utils::FilePath &file, ReplType type);
Utils::FilePath detectPython(const Utils::FilePath &documentPath);
QString pythonName(const Utils::FilePath &pythonPath);

class PythonProject;
PythonProject *pythonProjectForFile(const Utils::FilePath &pythonFile);

} // Python::Internal
