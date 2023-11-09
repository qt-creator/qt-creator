// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

namespace Utils { class FilePath; }

namespace Python::Internal {

const char PythonMimeType[] = "text/x-python-project";
const char PythonMimeTypeLegacy[] = "text/x-pyqt-project";
const char PythonProjectId[] = "PythonProject";
const char PythonErrorTaskCategory[] = "Task.Category.Python";

class PythonFileNode : public ProjectExplorer::FileNode
{
public:
    PythonFileNode(const Utils::FilePath &filePath,
                   const QString &nodeDisplayName,
                   ProjectExplorer::FileType fileType = ProjectExplorer::FileType::Source);

    QString displayName() const override;
private:
    QString m_displayName;
};

class PythonProjectNode : public ProjectExplorer::ProjectNode
{
public:
    explicit PythonProjectNode(const Utils::FilePath &path);
};

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
