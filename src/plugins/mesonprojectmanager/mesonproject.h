// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonprojectimporter.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectimporter.h>
#include <projectexplorer/task.h>

namespace MesonProjectManager {
namespace Internal {

class MesonProject final : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    explicit MesonProject(const Utils::FilePath &path);
    ~MesonProject() final = default;

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::ProjectImporter *projectImporter() const final;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

    mutable std::unique_ptr<MesonProjectImporter> m_projectImporter;
};

} // namespace Internal
} // namespace MesonProjectManager
