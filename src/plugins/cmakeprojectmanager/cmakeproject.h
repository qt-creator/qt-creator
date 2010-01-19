/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

struct CMakeTarget
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

    CMakeBuildConfiguration *activeCMakeBuildConfiguration() const;

    virtual QString displayName() const;
    virtual QString id() const;
    virtual Core::IFile *file() const;
    virtual ProjectExplorer::IBuildConfigurationFactory *buildConfigurationFactory() const;
    virtual CMakeManager *projectManager() const;

    virtual QList<ProjectExplorer::Project *> dependsOn(); //NBS TODO implement dependsOn

    virtual bool isApplication() const;

    virtual ProjectExplorer::BuildConfigWidget *createConfigWidget();
    virtual QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    virtual ProjectExplorer::ProjectNode *rootProjectNode() const;

    virtual QStringList files(FilesMode fileMode) const;
    QStringList targets() const;
    bool hasTarget(const QString &title) const;

    CMakeTarget targetForTitle(const QString &title);

    QString sourceDirectory() const;

signals:
    /// convenience signal emitted if the activeBuildConfiguration emits environmentChanged
    /// or if the activeBuildConfiguration changes
    void environmentChanged();
    /// emitted after parsing
    void targetsChanged();

protected:
    virtual bool fromMap(const QVariantMap &map);

    // called by CMakeBuildSettingsWidget
    void changeBuildDirectory(CMakeBuildConfiguration *bc, const QString &newBuildDirectory);

private slots:
    void fileChanged(const QString &fileName);
    void slotActiveBuildConfiguration();

private:
    bool parseCMakeLists();

    void buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    void gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list);
    ProjectExplorer::FolderNode *findOrCreateFolder(CMakeProjectNode *rootNode, QString directory);

    CMakeManager *m_manager;
    QString m_fileName;
    CMakeFile *m_file;
    QString m_projectName;
    CMakeBuildConfigurationFactory *m_buildConfigurationFactory;

    // TODO probably need a CMake specific node structure
    CMakeProjectNode *m_rootNode;
    QStringList m_files;
    QList<CMakeTarget> m_targets;
    ProjectExplorer::FileWatcher *m_watcher;
    bool m_insideFileChanged;
    QSet<QString> m_watchedFiles;
    CMakeBuildConfiguration *m_lastActiveBuildConfiguration;

    friend class CMakeBuildConfigurationFactory; // for parseCMakeLists
};

class CMakeCbpParser : public QXmlStreamReader
{
public:
    bool parseCbpFile(const QString &fileName);
    QList<ProjectExplorer::FileNode *> fileList();
    QList<ProjectExplorer::FileNode *> cmakeFileList();
    QStringList includeFiles();
    QList<CMakeTarget> targets();
    QString projectName() const;
    QString compilerName() const;
    bool hasCMakeFiles();
private:
    void parseCodeBlocks_project_file();
    void parseProject();
    void parseBuild();
    void parseOption();
    void parseTarget();
    void parseTargetOption();
    void parseMakeCommand();
    void parseTargetBuild();
    void parseTargetClean();
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

    CMakeTarget m_target;
    bool m_targetType;
    QList<CMakeTarget> m_targets;
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

    void modified(ReloadBehavior *behavior);
private:
    CMakeProject *m_project;
    QString m_fileName;
};

class CMakeBuildSettingsWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT
public:
    CMakeBuildSettingsWidget(CMakeProject *project);
    virtual QString displayName() const;

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
