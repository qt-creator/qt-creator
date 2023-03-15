// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"
#include "presetsparser.h"

#include <projectexplorer/project.h>

namespace CMakeProjectManager {

namespace Internal { class CMakeProjectImporter; }

class CMAKE_EXPORT CMakeProject final : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit CMakeProject(const Utils::FilePath &filename);
    ~CMakeProject() final;

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

    ProjectExplorer::ProjectImporter *projectImporter() const final;

    using IssueType = ProjectExplorer::Task::TaskType;
    void addIssue(IssueType type, const QString &text);
    void clearIssues();

    Internal::PresetsData presetsData() const;
    void readPresets();

protected:
    bool setupTarget(ProjectExplorer::Target *t) final;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;
    void configureAsExampleProject(ProjectExplorer::Kit *kit) override;

    Internal::PresetsData combinePresets(Internal::PresetsData &cmakePresetsData,
                                         Internal::PresetsData &cmakeUserPresetsData);
    void setupBuildPresets(Internal::PresetsData &presetsData);

    mutable Internal::CMakeProjectImporter *m_projectImporter = nullptr;

    ProjectExplorer::Tasks m_issues;
    Internal::PresetsData m_presetsData;
};

} // namespace CMakeProjectManager
