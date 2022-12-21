// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <QElapsedTimer>
#include <QTimer>

namespace Nim {

class NimBuildSystem;

class NimProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit NimProject(const Utils::FilePath &fileName);

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

    // Keep for compatibility with Qt Creator 4.10
    QVariantMap toMap() const final;

    QStringList excludedFiles() const;
    void setExcludedFiles(const QStringList &excludedFiles);

protected:
    // Keep for compatibility with Qt Creator 4.10
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) final;

    QStringList m_excludedFiles;
};

} // namespace Nim
