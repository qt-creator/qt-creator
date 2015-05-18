/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CMAKEPROJECT_H
#define CMAKEPROJECT_H

#include "cmake_global.h"
#include "cmakeprojectnodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <utils/fileutils.h>

#include <QFuture>
#include <QXmlStreamReader>
#include <QPushButton>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace CMakeProjectManager {

namespace Internal {
class CMakeFile;
class CMakeBuildSettingsWidget;
class CMakeBuildConfiguration;
class CMakeProjectNode;
class CMakeManager;
}

enum TargetType {
    ExecutableType = 0,
    StaticLibraryType = 2,
    DynamicLibraryType = 3
};

struct CMAKE_EXPORT CMakeBuildTarget
{
    QString title;
    QString executable; // TODO: rename to output?
    TargetType targetType;
    QString workingDirectory;
    QString sourceDirectory;
    QString makeCommand;
    QString makeCleanCommand;

    // code model
    QStringList includeFiles;
    QStringList compilerOptions;
    QByteArray defines;
    QStringList files;

    void clear();
};

class CMAKE_EXPORT CMakeProject : public ProjectExplorer::Project
{
    Q_OBJECT
    // for changeBuildDirectory
    friend class Internal::CMakeBuildSettingsWidget;
public:
    CMakeProject(Internal::CMakeManager *manager, const Utils::FileName &filename);
    ~CMakeProject();

    QString displayName() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;

    ProjectExplorer::ProjectNode *rootProjectNode() const;

    QStringList files(FilesMode fileMode) const;
    QStringList buildTargetTitles(bool runnable = false) const;
    QList<CMakeBuildTarget> buildTargets() const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    bool isProjectFile(const Utils::FileName &fileName);

    bool parseCMakeLists();

signals:
    /// emitted after parsing
    void buildTargetsChanged();

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);
    bool setupTarget(ProjectExplorer::Target *t);

    // called by CMakeBuildSettingsWidget
    void changeBuildDirectory(Internal::CMakeBuildConfiguration *bc, const QString &newBuildDirectory);

private slots:
    void fileChanged(const QString &fileName);
    void activeTargetWasChanged(ProjectExplorer::Target *target);
    void changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*);

    void updateRunConfigurations();

private:
    void buildTree(Internal::CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    void gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list);
    ProjectExplorer::FolderNode *findOrCreateFolder(Internal::CMakeProjectNode *rootNode, QString directory);
    void createUiCodeModelSupport();
    QString uiHeaderFile(const QString &uiFile);
    void updateRunConfigurations(ProjectExplorer::Target *t);
    void updateApplicationAndDeploymentTargets();
    QStringList getCXXFlagsFor(const CMakeBuildTarget &buildTarget);

    Internal::CMakeManager *m_manager;
    ProjectExplorer::Target *m_activeTarget;
    Utils::FileName m_fileName;
    Internal::CMakeFile *m_file;
    QString m_projectName;

    // TODO probably need a CMake specific node structure
    Internal::CMakeProjectNode *m_rootNode;
    QStringList m_files;
    QList<CMakeBuildTarget> m_buildTargets;
    QFileSystemWatcher *m_watcher;
    QSet<Utils::FileName> m_watchedFiles;
    QFuture<void> m_codeModelFuture;
};

} // namespace CMakeProjectManager

#endif // CMAKEPROJECT_H
