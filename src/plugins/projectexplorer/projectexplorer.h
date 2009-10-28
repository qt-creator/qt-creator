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

#include "projectexplorer_export.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QSharedPointer>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QPoint;
class QAction;
class QComboBox;
QT_END_NAMESPACE

namespace Core {
class IContext;
class IMode;
class IFileFactory;
}

namespace Utils {
class ParameterAction;
}

namespace ProjectExplorer {
class BuildManager;
class RunControl;
class SessionManager;
class RunConfiguration;
class IRunControlFactory;
class Project;
class Node;
class BuildConfiguration;

namespace Internal {
class ProjectFileFactory;
struct ProjectExplorerSettings;

class BuildConfigDialog : public QDialog
{
    Q_OBJECT
public:
    enum DialogResult {
        ChangeBuild = 10,
        Cancel = 11,
        Continue = 12
    };
    BuildConfigDialog(Project *project, QWidget *parent = 0);

    BuildConfiguration *selectedBuildConfiguration() const;

private slots:
    void buttonClicked();

private:
    Project *m_project;
    QPushButton *m_changeBuildConfiguration;
    QPushButton *m_cancel;
    QPushButton *m_justContinue;
    QComboBox *m_configCombo;
};

} // namespace Internal

struct ProjectExplorerPluginPrivate;

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
    void executeRunConfiguration(const QSharedPointer<RunConfiguration> &, const QString &mode);
    bool showBuildConfigDialog();
    void setCurrent(Project *project, QString filePath, Node *node);

    QStringList allFilesWithDependencies(Project *pro);
    IRunControlFactory *findRunControlFactory(const QSharedPointer<RunConfiguration> &config, const QString &mode);

    void updateActions();
    void addToRecentProjects(const QString &fileName, const QString &displayName);
    void updateWelcomePage();
    Internal::ProjectFileFactory *findProjectFileFactory(const QString &filename) const;

    static ProjectExplorerPlugin *m_instance;
    ProjectExplorerPluginPrivate *d;  
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_H
