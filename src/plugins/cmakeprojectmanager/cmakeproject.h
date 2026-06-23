// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"
#include "cmakespecificsettings.h"
#include "presetsparser.h"

#include <projectexplorer/project.h>
#include <utils/result.h>

namespace CMakeProjectManager {

namespace Internal {

QString packageManagerDir();

class CMakeProjectImporter;

} // namespace CMakeProjectManager::Internal

class CMAKE_EXPORT CMakeProject final : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit CMakeProject(const Utils::FilePath &filename);

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

    using IssueType = ProjectExplorer::Task::TaskType;
    void addIssue(IssueType type, const QString &text);
    void clearIssues();

    Internal::PresetsData presetsData() const;
    void readPresets();
    Utils::FilePath buildDirectoryToImport() const;
    void createKitsFromPresets() const;

    Internal::CMakeSpecificSettings &settings();
    static QString projectDisplayName(const Utils::FilePath &projectFilePath);

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

    Internal::PresetsData combinePresets(Internal::PresetsData &cmakePresetsData,
                                         Internal::PresetsData &cmakeUserPresetsData);
    void setupBuildPresets(Internal::PresetsData &presetsData);
    void setupTestPresets(Internal::PresetsData &presetsData);

    ProjectExplorer::Tasks m_issues;
    Internal::PresetsData m_presetsData;
    Internal::CMakeSpecificSettings m_settings;
    Utils::FilePath m_buildDirToImport;
    std::vector<Utils::Result<std::unique_ptr<Utils::FilePathWatcher>>> m_includeFilesWatcher;
};

#ifdef WITH_TESTS
QObject *createTestPresetsInheritanceTest();
#endif

} // namespace CMakeProjectManager
