// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>

namespace Python::Internal {

const char PythonMimeType[] = "text/x-python-project";
const char PythonMimeTypeLegacy[] = "text/x-pyqt-project";
const char PythonProjectId[] = "PythonProject";
const char PythonErrorTaskCategory[] = "Task.Category.Python";

class PythonProject : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    explicit PythonProject(const Utils::FilePath &filename);

    bool needsConfiguration() const final { return false; }

private:
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) override;
};

} // Python::Internal
