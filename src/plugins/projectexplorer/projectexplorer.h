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

#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include "project.h"
#include "session.h"
#include "projectexplorer_export.h"

#include <extensionsystem/iplugin.h>
#include <coreplugin/icorelistener.h>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#include <QtCore/QQueue>
#include <QtCore/QModelIndex>
#include <QtGui/QMenu>
#include <QtGui/QTreeWidget>
#include <QtGui/QTreeWidgetItem>

namespace Core {
class IContext;
class IMode;
class IFileFactory;
    namespace Utils {
        class ParameterAction;
    }
}

namespace Welcome {
    class WelcomeMode;
}

namespace ProjectExplorer {
class BuildManager;
class PersistentSettings;
class RunConfiguration;
class RunControl;
class SessionManager;
class IRunConfigurationRunner;

namespace Internal {
class ApplicationOutput;
class OutputPane;
class ProjectWindow;
class ProjectFileFactory;
class ProjectWelcomePage;
class ProjectWelcomePageWidget;

struct ProjectExplorerSettings
{
    bool buildBeforeRun;
    bool saveBeforeBuild;
    bool showCompilerOutput;
    bool useJom;
    bool operator==(const ProjectExplorerSettings &other) {
        return this->buildBeforeRun == other.buildBeforeRun
                && this->saveBeforeBuild == other.saveBeforeBuild
                && this->showCompilerOutput == other.showCompilerOutput
                && this->useJom == other.useJom;
    }
};

} // namespace Internal

class PROJECTEXPLORER_EXPORT ProjectExplorerPlugin
    : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    ProjectExplorerPlugin();
    ~ProjectExplorerPlugin();

    static ProjectExplorerPlugin *instance();

    bool openProject(const QString &fileName);
    QList<Project *> openProjects(const QStringList &fileNames);

    SessionManager *session() const;

    Project *currentProject() const;
    Node *currentNode() const;

    void setCurrentFile(Project *project, const QString &file);
    void setCurrentNode(Node *node);

    Project *startupProject() const;
    void setStartupProject(ProjectExplorer::Project *project = 0);

    BuildManager *buildManager() const;

    bool saveModifiedFiles();

    void showContextMenu(const QPoint &globalPos, Node *node);

    //PluginInterface
    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    void shutdown();

    void setProjectExplorerSettings(const Internal::ProjectExplorerSettings &pes);
    Internal::ProjectExplorerSettings projectExplorerSettings() const;

signals:
    void aboutToShowContextMenu(ProjectExplorer::Project *project,
                                ProjectExplorer::Node *node);

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged();

    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void aboutToExecuteProject(ProjectExplorer::Project *project);

    void settingsChanged();

private slots:
    void buildStateChanged(ProjectExplorer::Project * pro);
    void buildQueueFinished(bool success);
    void buildProjectOnly();
    void buildProject();
    void buildSession();
    void rebuildProjectOnly();
    void rebuildProject();
    void rebuildSession();
    void cleanProjectOnly();
    void cleanProject();
    void cleanSession();
    void cancelBuild();
    void debugProject();
    void loadAction();
    void unloadProject();
    void clearSession();
    void newProject();
    void showSessionManager();
    void populateBuildConfigurationMenu();
    void buildConfigurationMenuTriggered(QAction *);
    void populateRunConfigurationMenu();
    void runConfigurationMenuTriggered(QAction *);
    void populateOpenWithMenu();
    void openWithMenuTriggered(QAction *action);
    void updateSessionMenu();
    void setSession(QAction *action);

    void determineSessionToRestoreAtStartup();
    void restoreSession();
    void loadSession(const QString &session);
    void runProject();
    void runProjectContextMenu();
    void savePersistentSettings();
    void goToTaskWindow();

    void updateContextMenuActions();
    void addNewFile();
    void addExistingFiles();
    void openFile();
    void showInGraphicalShell();
    void removeFile();
    void renameFile();

    void updateRecentProjectMenu();
    void openRecentProject();

    void invalidateProject(ProjectExplorer::Project *project);

    void setCurrentFile(const QString &filePath);

    // RunControl
    void runControlFinished();

    void startupProjectChanged(); // Calls updateRunAction
    void updateRunAction();

    void addToApplicationOutputWindow(RunControl *, const QString &line);
    void addToApplicationOutputWindowInline(RunControl *, const QString &line);
    void addErrorToApplicationOutputWindow(RunControl *, const QString &error);

    void loadProject(const QString &project) { openProject(project); }
    void currentModeChanged(Core::IMode *mode);

private:
    void runProjectImpl(Project *pro);
    void executeRunConfiguration(QSharedPointer<RunConfiguration>, const QString &mode);
    void setCurrent(Project *project, QString filePath, Node *node);

    QStringList allFilesWithDependencies(Project *pro);
    IRunConfigurationRunner *findRunner(QSharedPointer<RunConfiguration> config, const QString &mode);

    void updateActions();
    void addToRecentProjects(const QString &fileName, const QString &displayName);
    void updateWelcomePage();
    Internal::ProjectFileFactory *findProjectFileFactory(const QString &filename) const;

    static ProjectExplorerPlugin *m_instance;

    QMenu *m_sessionContextMenu;
    QMenu *m_sessionMenu;
    QMenu *m_projectMenu;
    QMenu *m_subProjectMenu;
    QMenu *m_folderMenu;
    QMenu *m_fileMenu;
    QMenu *m_openWithMenu;

    QMultiMap<int, QObject*> m_actionMap;
    QAction *m_sessionManagerAction;
    QAction *m_newAction;
#if 0
    QAction *m_loadAction;
#endif
    Core::Utils::ParameterAction *m_unloadAction;
    QAction *m_clearSession;
    QAction *m_buildProjectOnlyAction;
    Core::Utils::ParameterAction *m_buildAction;
    QAction *m_buildSessionAction;
    QAction *m_rebuildProjectOnlyAction;
    Core::Utils::ParameterAction *m_rebuildAction;
    QAction *m_rebuildSessionAction;
    QAction *m_cleanProjectOnlyAction;
    Core::Utils::ParameterAction *m_cleanAction;
    QAction *m_cleanSessionAction;
    QAction *m_runAction;
    QAction *m_runActionContextMenu;
    QAction *m_cancelBuildAction;
    QAction *m_debugAction;
    QAction *m_addNewFileAction;
    QAction *m_addExistingFilesAction;
    QAction *m_openFileAction;
    QAction *m_showInGraphicalShell;
    QAction *m_removeFileAction;
    QAction *m_renameFileAction;

    QMenu *m_buildConfigurationMenu;
    QActionGroup *m_buildConfigurationActionGroup;
    QMenu *m_runConfigurationMenu;
    QActionGroup *m_runConfigurationActionGroup;

    Internal::ProjectWindow *m_proWindow;
    SessionManager *m_session;
    QString m_sessionToRestoreAtStartup;

    Project *m_currentProject;
    Node *m_currentNode;

    BuildManager *m_buildManager;

    QList<Internal::ProjectFileFactory*> m_fileFactories;
    QStringList m_profileMimeTypes;
    Internal::OutputPane *m_outputPane;

    QList<QPair<QString, QString> > m_recentProjects; // pair of filename, displayname
    static const int m_maxRecentProjects = 7;

    QString m_lastOpenDirectory;
    QSharedPointer<RunConfiguration> m_delayedRunConfiguration;
    RunControl *m_debuggingRunControl;
    QString m_runMode;
    QString m_projectFilterString;
    Internal::ProjectExplorerSettings m_projectExplorerSettings;
    Internal::ProjectWelcomePage *m_welcomePlugin;
    Internal::ProjectWelcomePageWidget *m_welcomePage;
};

namespace Internal {

class CoreListenerCheckingForRunningBuild : public Core::ICoreListener
{
    Q_OBJECT
public:
    CoreListenerCheckingForRunningBuild(BuildManager *manager);

    bool coreAboutToClose();

private:
    BuildManager *m_manager;
};

} // namespace Internal

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_H
