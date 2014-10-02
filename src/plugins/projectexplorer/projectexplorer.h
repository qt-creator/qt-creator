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

#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include "projectexplorer_export.h"
#include "projectexplorerconstants.h"

#include <extensionsystem/iplugin.h>

#include <QPair>

QT_BEGIN_NAMESPACE
class QPoint;
class QAction;
QT_END_NAMESPACE

namespace Core {
class IMode;
class Id;
}

namespace ProjectExplorer {
class RunControl;
class RunConfiguration;
class IRunControlFactory;
class Project;
class Node;
class FolderNode;
class FileNode;

namespace Internal { class ProjectExplorerSettings; }

class PROJECTEXPLORER_EXPORT ProjectExplorerPlugin
    : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ProjectExplorer.json")

public:
    ProjectExplorerPlugin();
    ~ProjectExplorerPlugin();

    static ProjectExplorerPlugin *instance();

    static Project *openProject(const QString &fileName, QString *error);
    static QList<Project *> openProjects(const QStringList &fileNames, QString *error);
    Q_SLOT void openProjectWelcomePage(const QString &fileName);
    static void unloadProject(Project *project);

    static Project *currentProject();
    static Node *currentNode();

    static void setCurrentFile(Project *project, const QString &file);
    static void setCurrentNode(Node *node);

    static bool saveModifiedFiles();

    static void showContextMenu(QWidget *view, const QPoint &globalPos, Node *node);

    //PluginInterface
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    static void setProjectExplorerSettings(const Internal::ProjectExplorerSettings &pes);
    static Internal::ProjectExplorerSettings projectExplorerSettings();

    static void startRunControl(RunControl *runControl, RunMode runMode);
    static void showRunErrorMessage(const QString &errorMessage);

    // internal public for FlatModel
    static void renameFile(Node *node, const QString &to);
    static QStringList projectFilePatterns();
    static bool coreAboutToClose();
    static QList<QPair<QString, QString> > recentProjects();

    static bool canRun(Project *pro, RunMode runMode, QString *whyNot = 0);
    static void runProject(Project *pro, RunMode, const bool forceSkipDeploy = false);
    static void runRunConfiguration(RunConfiguration *rc, RunMode runMode,
                             const bool forceSkipDeploy = false);

    static void addExistingFiles(FolderNode *projectNode, const QStringList &filePaths);
    static void addExistingFiles(const QStringList &filePaths);

    static void buildProject(Project *p);
    /// Normally there's no need to call this function.
    /// This function needs to be called, only if the pages that support a project changed.
    static void requestProjectModeUpdate(Project *p);

    static void initiateInlineRenaming();

    static QString displayNameForStepId(Core::Id stepId);

    static QString directoryFor(Node *node);
    static QStringList projectFileGlobs();

signals:
    void runControlStarted(ProjectExplorer::RunControl *rc);
    void runControlFinished(ProjectExplorer::RunControl *rc);
    void aboutToShowContextMenu(ProjectExplorer::Project *project,
                                ProjectExplorer::Node *node);

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged();

    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void aboutToExecuteProject(ProjectExplorer::Project *project, RunMode runMode);
    void recentProjectsChanged();

    void settingsChanged();

    void updateRunActions();

public slots:
    static void openOpenProjectDialog();

private slots:
    void buildStateChanged(ProjectExplorer::Project * pro);
    void buildQueueFinished(bool success);
    void buildProjectOnly();
    void buildProject();
    void buildProjectContextMenu();
    void buildSession();
    void rebuildProjectOnly();
    void rebuildProject();
    void rebuildProjectContextMenu();
    void rebuildSession();
    void deployProjectOnly();
    void deployProject();
    void deployProjectContextMenu();
    void deploySession();
    void cleanProjectOnly();
    void cleanProject();
    void cleanProjectContextMenu();
    void cleanSession();
    void cancelBuild();
    void loadAction();
    void unloadProject();
    void closeAllProjects();
    void newProject();
    void showSessionManager();
    void populateOpenWithMenu();
    void updateSessionMenu();
    void setSession(QAction *action);

    void determineSessionToRestoreAtStartup();
    void restoreSession();
    void restoreSession2();
    void loadSession(const QString &session);
    void runProject();
    void runProjectWithoutDeploy();
    void runProjectContextMenu();
    void savePersistentSettings();

    void addNewFile();
    void addExistingFiles();
    void addExistingDirectory();
    void addNewSubproject();
    void removeProject();
    void openFile();
    void searchOnFileSystem();
    void showInGraphicalShell();
    void removeFile();
    void deleteFile();
    void renameFile();
    void setStartupProject();
    void setStartupProject(ProjectExplorer::Project *project);

    void updateRecentProjectMenu();
    void clearRecentProjects();
    void openRecentProject();
    void updateUnloadProjectMenu();
    void openTerminalHere();

    // for keeping current node / current project up to date
    void invalidateProject(ProjectExplorer::Project *project);
    void foldersAboutToBeRemoved(FolderNode *, const QList<FolderNode*> &);
    void filesAboutToBeRemoved(FolderNode *, const QList<FileNode*> &);

    void setCurrentFile(const QString &filePath);

    void runControlFinished();

    void projectAdded(ProjectExplorer::Project *pro);
    void projectRemoved(ProjectExplorer::Project *pro);
    void projectDisplayNameChanged(ProjectExplorer::Project *pro);
    void startupProjectChanged(); // Calls updateRunAction
    void activeTargetChanged();
    void activeRunConfigurationChanged();

    void slotUpdateRunActions();

    void currentModeChanged(Core::IMode *mode, Core::IMode *oldMode);
    void loadCustomWizards();

    void updateWelcomePage();
    void updateExternalFileWarning();

    void updateActions();
    void updateContext();
    void runConfigurationConfigurationFinished();

#ifdef WITH_TESTS
    void testAnsiFilterOutputParser_data();
    void testAnsiFilterOutputParser();

    void testGccOutputParsers_data();
    void testGccOutputParsers();

    void testCustomOutputParsers_data();
    void testCustomOutputParsers();

    void testClangOutputParser_data();
    void testClangOutputParser();

    void testLinuxIccOutputParsers_data();
    void testLinuxIccOutputParsers();

    void testGnuMakeParserParsing_data();
    void testGnuMakeParserParsing();
    void testGnuMakeParserTaskMangling_data();
    void testGnuMakeParserTaskMangling();

    void testXcodebuildParserParsing_data();
    void testXcodebuildParserParsing();

    void testMsvcOutputParsers_data();
    void testMsvcOutputParsers();

    void testGccAbiGuessing_data();
    void testGccAbiGuessing();

    void testAbiOfBinary_data();
    void testAbiOfBinary();
    void testFlavorForOs();
    void testAbiFromTargetTriplet_data();
    void testAbiFromTargetTriplet();

    void testDeviceManager();

    void testCustomWizardPreprocessor_data();
    void testCustomWizardPreprocessor();
#endif
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_H
