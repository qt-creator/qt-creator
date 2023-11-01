// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "buildsystem/qmlbuildsystem.h" // IWYU pragma: keep
#include "qmlprojectmanager_global.h"
#include <projectexplorer/project.h>

#include <QPointer>

namespace QmlProjectManager {

class QmlProject;

class QMLPROJECTMANAGER_EXPORT QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    explicit QmlProject(const Utils::FilePath &filename);

    static bool isQtDesignStudioStartedFromQtC();
    bool isEditModePreferred() const override;

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

protected:
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) override;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;
    Utils::FilePaths collectUiQmlFilesForFolder(const Utils::FilePath &folder) const;
    Utils::FilePaths collectQmlFiles() const;

    bool setKitWithVersion(const int qtMajorVersion, const QList<ProjectExplorer::Kit *> kits);

    bool allowOnlySingleProject();
    int preferedQtTarget(ProjectExplorer::Target *target);

private slots:
    void parsingFinished(const ProjectExplorer::Target *target, bool success);
};

class FilesUpdateBlocker
{
public:
    FilesUpdateBlocker(QmlBuildSystem *bs)
        : m_bs(bs)
    {
        if (m_bs)
            m_bs->m_blockFilesUpdate = true;
    }

    ~FilesUpdateBlocker()
    {
        if (m_bs) {
            m_bs->m_blockFilesUpdate = false;
            m_bs->refresh(QmlBuildSystem::RefreshOptions::Project);
        }
    }

private:
    QPointer<QmlBuildSystem> m_bs;
};

} // namespace QmlProjectManager
