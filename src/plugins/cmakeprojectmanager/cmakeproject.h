/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
class CMakeUiCodeModelSupport;

struct CMakeBuildTarget
{
    QString title;
    QString executable; // TODO: rename to output?
    bool library;
    QString workingDirectory;
    QString makeCommand;
    QString makeCleanCommand;
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
    Core::Id id() const;
    Core::IDocument *document() const;
    CMakeManager *projectManager() const;

    ProjectExplorer::ProjectNode *rootProjectNode() const;

    QStringList files(FilesMode fileMode) const;
    QStringList buildTargetTitles() const;
    QList<CMakeBuildTarget> buildTargets() const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    QString shadowBuildDirectory(const QString &projectFilePath, const ProjectExplorer::Kit *k,
                                 const QString &bcName);

    QString uicCommand() const;

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

    void editorChanged(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void uiEditorContentsChanged();
    void buildStateChanged(ProjectExplorer::Project *project);
    void updateRunConfigurations();

private:
    void buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    void gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list);
    ProjectExplorer::FolderNode *findOrCreateFolder(CMakeProjectNode *rootNode, QString directory);
    void updateCodeModelSupportFromEditor(const QString &uiFileName, const QString &contents);
    void createUiCodeModelSupport();
    QString uiHeaderFile(const QString &uiFile);
    void updateRunConfigurations(ProjectExplorer::Target *t);

    CMakeManager *m_manager;
    ProjectExplorer::Target *m_activeTarget;
    QString m_fileName;
    CMakeFile *m_file;
    QString m_projectName;
    QString m_uicCommand;

    // TODO probably need a CMake specific node structure
    CMakeProjectNode *m_rootNode;
    QStringList m_files;
    QList<CMakeBuildTarget> m_buildTargets;
    QFileSystemWatcher *m_watcher;
    QSet<QString> m_watchedFiles;
    QFuture<void> m_codeModelFuture;

    QMap<QString, CMakeUiCodeModelSupport *> m_uiCodeModelSupport;
    Core::IEditor *m_lastEditor;
    bool m_dirtyUic;
};

class CMakeCbpParser : public QXmlStreamReader
{
public:
    bool parseCbpFile(const QString &fileName);
    QList<ProjectExplorer::FileNode *> fileList();
    QList<ProjectExplorer::FileNode *> cmakeFileList();
    QStringList includeFiles();
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

    QList<ProjectExplorer::FileNode *> m_fileList;
    QList<ProjectExplorer::FileNode *> m_cmakeFileList;
    QSet<QString> m_processedUnits;
    bool m_parsingCmakeUnit;
    QStringList m_includeFiles;
    QStringList m_compilerOptions;

    CMakeBuildTarget m_buildTarget;
    bool m_buildTargetType;
    QList<CMakeBuildTarget> m_buildTargets;
    QString m_projectName;
    QString m_compiler;
};

class CMakeFile : public Core::IDocument
{
    Q_OBJECT
public:
    CMakeFile(CMakeProject *parent, QString fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

    void rename(const QString &newName);

private:
    CMakeProject *m_project;
    QString m_fileName;
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
