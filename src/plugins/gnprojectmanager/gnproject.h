// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>

namespace GNProjectManager::Internal {

void setupGNProject();

class GNProject final : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    explicit GNProject(const Utils::FilePath &path);
    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;
    using IssueType = ProjectExplorer::Task::TaskType;
    void addIssue(IssueType type, const QString &text);
    void clearIssues();

private:
    ProjectExplorer::Tasks m_issues;
};

} // namespace GNProjectManager::Internal
