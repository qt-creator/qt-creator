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

#include "qmlprojectmanager.h"
#include "qmlprojectnodes.h"

#include <projectexplorer/project.h>

#include <QPointer>

namespace ProjectExplorer { class RunConfiguration; }
namespace QmlJS { class ModelManagerInterface; }
namespace Utils { class FileSystemWatcher; }

namespace QmlProjectManager {

class QmlProjectItem;

namespace Internal { class QmlProjectFile; }

class QMLPROJECTMANAGER_EXPORT QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    QmlProject(Internal::Manager *manager, const Utils::FileName &filename);
    ~QmlProject() override;

    Utils::FileName filesFileName() const;

    QString displayName() const override;
    Internal::Manager *projectManager() const override;

    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const override;

    Internal::QmlProjectNode *rootProjectNode() const override;
    QStringList files(FilesMode fileMode) const override;

    bool validProjectFile() const;

    enum RefreshOption {
        ProjectFile   = 0x01,
        Files         = 0x02,
        Configuration = 0x04,
        Everything    = ProjectFile | Files | Configuration
    };
    Q_DECLARE_FLAGS(RefreshOptions,RefreshOption)

    void refresh(RefreshOptions options);

    QDir projectDir() const;
    QStringList files() const;
    QString mainFile() const;
    QStringList customImportPaths() const;

    bool addFiles(const QStringList &filePaths);

    void refreshProjectFile();

    enum QmlImport { UnknownImport, QtQuick1Import, QtQuick2Import };
    QmlImport defaultImport() const;

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    void refreshFiles(const QSet<QString> &added, const QSet<QString> &removed);
    void addedTarget(ProjectExplorer::Target *target);
    void onActiveTargetChanged(ProjectExplorer::Target *target);
    void onKitChanged();
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *);

    // plain format
    void parseProject(RefreshOptions options);
    QStringList convertToAbsoluteFiles(const QStringList &paths) const;
    QmlJS::ModelManagerInterface *modelManager() const;

    QString m_projectName;
    QmlImport m_defaultImport;
    ProjectExplorer::Target *m_activeTarget = 0;

    // plain format
    QStringList m_files;

    QPointer<QmlProjectItem> m_projectItem;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlProject::RefreshOptions)
