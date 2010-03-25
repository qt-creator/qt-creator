/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CMAKEPROJECT_H
#define CMAKEPROJECT_H

#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "cmakebuildconfiguration.h"
#include "cmaketarget.h"
#include "makestep.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/filewatcher.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/ifile.h>

#include <QtCore/QXmlStreamReader>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>

namespace CMakeProjectManager {
namespace Internal {

class CMakeFile;
class CMakeBuildSettingsWidget;

struct CMakeBuildTarget
{
    QString title;
    QString executable;
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
    QString id() const;
    Core::IFile *file() const;
    CMakeTargetFactory *targetFactory() const;
    CMakeManager *projectManager() const;

    CMakeTarget *activeTarget() const;

    QList<ProjectExplorer::Project *> dependsOn(); //NBS TODO implement dependsOn

    bool isApplication() const;

    ProjectExplorer::BuildConfigWidget *createConfigWidget();
    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    ProjectExplorer::ProjectNode *rootProjectNode() const;

    QStringList files(FilesMode fileMode) const;
    QStringList buildTargetTitles() const;
    QList<CMakeBuildTarget> buildTargets() const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    bool parseCMakeLists();

signals:
    /// emitted after parsing
    void buildTargetsChanged();

protected:
    bool fromMap(const QVariantMap &map);

    // called by CMakeBuildSettingsWidget
    void changeBuildDirectory(CMakeBuildConfiguration *bc, const QString &newBuildDirectory);

private slots:
    void fileChanged(const QString &fileName);
    void changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*);
    void targetAdded(ProjectExplorer::Target *);

private:
    void buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    void gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list);
    ProjectExplorer::FolderNode *findOrCreateFolder(CMakeProjectNode *rootNode, QString directory);

    CMakeManager *m_manager;
    QString m_fileName;
    CMakeFile *m_file;
    QString m_projectName;

    // TODO probably need a CMake specific node structure
    CMakeProjectNode *m_rootNode;
    QStringList m_files;
    QList<CMakeBuildTarget> m_buildTargets;
    ProjectExplorer::FileWatcher *m_watcher;
    bool m_insideFileChanged;
    QSet<QString> m_watchedFiles;
    CMakeTargetFactory *m_targetFactory;
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
    void parseMakeCommand();
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

    CMakeBuildTarget m_buildTarget;
    bool m_buildTargetType;
    QList<CMakeBuildTarget> m_buildTargets;
    QString m_projectName;
    QString m_compiler;
};

class CMakeFile : public Core::IFile
{
    Q_OBJECT
public:
    CMakeFile(CMakeProject *parent, QString fileName);

    bool save(const QString &fileName = QString());
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);

private:
    CMakeProject *m_project;
    QString m_fileName;
};

class CMakeBuildSettingsWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT
public:
    explicit CMakeBuildSettingsWidget(CMakeProject *project);
    QString displayName() const;

    // This is called to set up the config widget before showing it
    // buildConfiguration is QString::null for the non buildConfiguration specific page
    virtual void init(ProjectExplorer::BuildConfiguration *bc);
private slots:
    void openChangeBuildDirectoryDialog();
private:
    CMakeProject *m_project;
    QLineEdit *m_pathLineEdit;
    QPushButton *m_changeButton;
    CMakeBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECT_H
