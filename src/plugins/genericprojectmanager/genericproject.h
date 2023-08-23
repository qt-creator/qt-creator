// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>

namespace GenericProjectManager {
namespace Internal {

class GenericProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit GenericProject(const Utils::FilePath &filename);

    void editFilesTriggered();
    void removeFilesTriggered(const Utils::FilePaths &filesToRemove);

private:
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) final;
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const final;
    void configureAsExampleProject(ProjectExplorer::Kit *kit) override;
};

} // namespace Internal
} // namespace GenericProjectManager
