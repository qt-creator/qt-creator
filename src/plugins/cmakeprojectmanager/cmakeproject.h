// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"
#include "cmakespecificsettings.h"
#include "presetsparser.h"

#include <projectexplorer/project.h>
#include <utils/id.h>
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
    const QList<Utils::Id> &presetKitIds() const;

    Internal::CMakeSpecificSettings &settings();
    static QString projectDisplayName(const Utils::FilePath &projectFilePath);

private:

    ProjectExplorer::Tasks m_issues;
    Internal::PresetsData m_presetsData;
    QList<Utils::Id> m_presetKitIds;
    bool m_presetsReloadPending = false;
    Internal::CMakeSpecificSettings m_settings;
    Utils::FilePath m_buildDirToImport;
    std::vector<Utils::Result<std::unique_ptr<Utils::FilePathWatcher>>> m_includeFilesWatcher;
};

#ifdef WITH_TESTS
QObject *createTestPresetsInheritanceTest();
#endif

} // namespace CMakeProjectManager
