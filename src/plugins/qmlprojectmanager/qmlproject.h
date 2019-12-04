/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmlprojectmanager_global.h"
#include "qmlprojectnodes.h"

#include <projectexplorer/project.h>

#include <utils/environment.h>

#include <QPointer>

namespace ProjectExplorer { class RunConfiguration; }

namespace QmlProjectManager {

class QmlProjectItem;

class QMLPROJECTMANAGER_EXPORT QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit QmlProject(const Utils::FilePath &filename);
    ~QmlProject() override;

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

    enum RefreshOption {
        ProjectFile   = 0x01,
        Files         = 0x02,
        Configuration = 0x04,
        Everything    = ProjectFile | Files | Configuration
    };
    Q_DECLARE_FLAGS(RefreshOptions,RefreshOption)

    void refresh(RefreshOptions options);

    Utils::FilePath canonicalProjectDir() const;
    QString mainFile() const;
    void setMainFile(const QString &mainFilePath);
    Utils::FilePath targetDirectory(const ProjectExplorer::Target *target) const;
    Utils::FilePath targetFile(const Utils::FilePath &sourceFile,
                               const ProjectExplorer::Target *target) const;

    Utils::EnvironmentItems environment() const;
    QStringList customImportPaths() const;
    QStringList customFileSelectors() const;
    bool forceFreeType() const;

    bool addFiles(const QStringList &filePaths);

    void refreshProjectFile();

    static QStringList makeAbsolute(const Utils::FilePath &path, const QStringList &relativePaths);

    QVariant additionalData(Core::Id id, const ProjectExplorer::Target *target) const override;

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

    void generateProjectTree();
    void updateDeploymentData(ProjectExplorer::Target *target);
    void refreshFiles(const QSet<QString> &added, const QSet<QString> &removed);
    void refreshTargetDirectory();
    void addedTarget(ProjectExplorer::Target *target);
    void onActiveTargetChanged(ProjectExplorer::Target *target);
    void onKitChanged();

    // plain format
    void parseProject(RefreshOptions options);

    ProjectExplorer::Target *m_activeTarget = nullptr;

    QPointer<QmlProjectItem> m_projectItem;
    Utils::FilePath m_canonicalProjectDir;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlProject::RefreshOptions)
