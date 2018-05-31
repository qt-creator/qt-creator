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
    explicit QmlProject(const Utils::FileName &filename);
    ~QmlProject() override;

    QList<ProjectExplorer::Task> projectIssues(const ProjectExplorer::Kit *k) const final;

    bool validProjectFile() const;

    enum RefreshOption {
        ProjectFile   = 0x01,
        Files         = 0x02,
        Configuration = 0x04,
        Everything    = ProjectFile | Files | Configuration
    };
    Q_DECLARE_FLAGS(RefreshOptions,RefreshOption)

    void refresh(RefreshOptions options);

    Utils::FileName canonicalProjectDir() const;
    QString mainFile() const;
    Utils::FileName targetDirectory(const ProjectExplorer::Target *target) const;
    Utils::FileName targetFile(const Utils::FileName &sourceFile,
                               const ProjectExplorer::Target *target) const;

    QList<Utils::EnvironmentItem> environment() const;
    QStringList customImportPaths() const;

    bool addFiles(const QStringList &filePaths);

    void refreshProjectFile();

    bool needsBuildConfigurations() const final;

    static QStringList makeAbsolute(const Utils::FileName &path, const QStringList &relativePaths);
protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
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
    Utils::FileName m_canonicalProjectDir;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlProject::RefreshOptions)
