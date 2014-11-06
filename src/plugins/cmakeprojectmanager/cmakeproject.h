/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CMAKEPROJECT_H
#define CMAKEPROJECT_H

#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "cmakebuildconfiguration.h"
#include "makestep.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QFuture>
#include <QXmlStreamReader>
#include <QPushButton>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace ProjectExplorer { class Target; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeFile;
class CMakeBuildSettingsWidget;

enum TargetType {
    ExecutableType = 0,
    StaticLibraryType = 2,
    DynamicLibraryType = 3
};

struct CMakeBuildTarget
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

class CMakeProject : public ProjectExplorer::Project
{
    Q_OBJECT
    // for changeBuildDirectory
    friend class CMakeBuildSettingsWidget;
public:
    CMakeProject(CMakeManager *manager, const QString &filename);
    ~CMakeProject();

    QString displayName() const;
    Core::IDocument *document() const;
    CMakeManager *projectManager() const;

    ProjectExplorer::ProjectNode *rootProjectNode() const;

    QStringList files(FilesMode fileMode) const;
    QStringList buildTargetTitles(bool runnable = false) const;
    QList<CMakeBuildTarget> buildTargets() const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    bool isProjectFile(const QString &fileName);

    bool parseCMakeLists();

signals:
    /// emitted after parsing
    void buildTargetsChanged();

protected:
    bool fromMap(const QVariantMap &map);
    bool setupTarget(ProjectExplorer::Target *t);

    // called by CMakeBuildSettingsWidget
    void changeBuildDirectory(CMakeBuildConfiguration *bc, const QString &newBuildDirectory);

private slots:
    void fileChanged(const QString &fileName);
    void activeTargetWasChanged(ProjectExplorer::Target *target);
    void changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*);

    void updateRunConfigurations();

private:
    void buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    void gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list);
    ProjectExplorer::FolderNode *findOrCreateFolder(CMakeProjectNode *rootNode, QString directory);
    void createUiCodeModelSupport();
    QString uiHeaderFile(const QString &uiFile);
    void updateRunConfigurations(ProjectExplorer::Target *t);
    void updateApplicationAndDeploymentTargets();
    QStringList getCXXFlagsFor(const CMakeBuildTarget &buildTarget);

    CMakeManager *m_manager;
    ProjectExplorer::Target *m_activeTarget;
    QString m_fileName;
    CMakeFile *m_file;
    QString m_projectName;

    // TODO probably need a CMake specific node structure
    CMakeProjectNode *m_rootNode;
    QStringList m_files;
    QList<CMakeBuildTarget> m_buildTargets;
    QFileSystemWatcher *m_watcher;
    QSet<QString> m_watchedFiles;
    QFuture<void> m_codeModelFuture;
};

class CMakeCbpParser : public QXmlStreamReader
{
public:
    bool parseCbpFile(const QString &fileName, const QString &sourceDirectory);
    QList<ProjectExplorer::FileNode *> fileList();
    QList<ProjectExplorer::FileNode *> cmakeFileList();
    QList<CMakeBuildTarget> buildTargets();
    QString projectName() const;
    QString compilerName() const;
    bool hasCMakeFiles();

private:
    void parseCodeBlocks_project_file();
    void parseProject();
    void parseBuild();
    void parseOption();
    void parseBuildTarget();
    void parseBuildTargetOption();
    void parseMakeCommands();
    void parseBuildTargetBuild();
    void parseBuildTargetClean();
    void parseCompiler();
    void parseAdd();
    void parseUnit();
    void parseUnitOption();
    void parseUnknownElement();
    void sortFiles();

    QList<ProjectExplorer::FileNode *> m_fileList;
    QList<ProjectExplorer::FileNode *> m_cmakeFileList;
    QSet<QString> m_processedUnits;
    bool m_parsingCmakeUnit;

    CMakeBuildTarget m_buildTarget;
    QList<CMakeBuildTarget> m_buildTargets;
    QString m_projectName;
    QString m_compiler;
    QString m_sourceDirectory;
    QString m_buildDirectory;
};

class CMakeFile : public Core::IDocument
{
    Q_OBJECT
public:
    CMakeFile(CMakeProject *parent, QString fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave);

    QString defaultPath() const;
    QString suggestedFileName() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    CMakeProject *m_project;
};

class CMakeBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT
public:
    CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc);

private slots:
    void openChangeBuildDirectoryDialog();
    void runCMake();
private:
    QLineEdit *m_pathLineEdit;
    QPushButton *m_changeButton;
    CMakeBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECT_H
