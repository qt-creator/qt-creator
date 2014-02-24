/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECT_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECT_H

#include "vcprojectmodel/vcprojectdocument_constants.h"
#include "msbuildversionmanager.h"

#include <projectexplorer/namedwidget.h>
#include <projectexplorer/project.h>

#include <QFuture>

class QFileSystemWatcher;

namespace ProjectExplorer {
class FolderNode;
}

namespace VcProjectManager {
namespace Internal {

class VcProjectFile;
class VcDocProjectNode;
class VcManager;
class Configuration;
class VcProjectBuildConfiguration;
class IConfiguration;
class IFile;

class VcProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    VcProject(VcManager *projectManager, const QString &projectFilePath, VcDocConstants::DocumentVersion docVersion);
    ~VcProject();

    QString displayName() const;
    Core::Id id() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    ProjectExplorer::ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;
    QString defaultBuildDirectory() const;
    bool needsConfiguration() const;
    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const;

public slots:
    void reloadProjectNodes();

private slots:
    void onSettingsDialogAccepted();

protected:
    bool fromMap(const QVariantMap &map);
    bool setupTarget(ProjectExplorer::Target *t);

private:
    void addCxxModelFiles(const ProjectExplorer::FolderNode *node, QStringList &sourceFiles);
    void updateCodeModels();
    void importBuildConfigurations();
    void addBuildConfiguration(ProjectExplorer::Target *target, IConfiguration *config);
    VcProjectBuildConfiguration *findBuildConfiguration(ProjectExplorer::Target *target, const QString &buildConfigurationName) const;
    void allProjectFile(QStringList &allFiles) const;
    VcManager *m_projectManager;
    VcProjectFile *m_projectFile;
    VcDocProjectNode *m_rootNode;
    QFuture<void> m_codeModelFuture;
};

class VcProjectBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
public:
    VcProjectBuildSettingsWidget();
    QString displayName() const;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECT_H
