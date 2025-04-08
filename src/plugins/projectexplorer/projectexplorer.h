// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <extensionsystem/iplugin.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/result.h>

QT_BEGIN_NAMESPACE
class QPoint;
class QThreadPool;
QT_END_NAMESPACE

namespace Core {
class OutputWindow;
} // Core

namespace ProjectExplorer {
class CustomParserSettings;
class FolderNode;
class Node;
class Project;
class RunControl;
class RunConfiguration;

namespace Internal {
class MiniProjectTargetSelector;
}

class RecentProjectsEntry
{
public:
    Utils::FilePath filePath;
    QString displayName;
    bool exists = true;
};

using RecentProjectsEntries = QList<RecentProjectsEntry>;

class PROJECTEXPLORER_EXPORT OpenProjectResult
{
public:
    OpenProjectResult(const QList<Project *> &projects, const QList<Project *> &alreadyOpen,
                      const QString &errorMessage)
        : m_projects(projects), m_alreadyOpen(alreadyOpen),
          m_errorMessage(errorMessage)
    { }

    explicit operator bool() const
    {
        return m_errorMessage.isEmpty() && m_alreadyOpen.isEmpty();
    }

    Project *project() const
    {
        return m_projects.isEmpty() ? nullptr : m_projects.first();
    }

    QList<Project *> projects() const
    {
        return m_projects;
    }

    QString errorMessage() const
    {
        return m_errorMessage;
    }

    QList<Project *> alreadyOpen() const
    {
        return m_alreadyOpen;
    }

private:
    QList<Project *> m_projects;
    QList<Project *> m_alreadyOpen;
    QString m_errorMessage;
};

class PROJECTEXPLORER_EXPORT ProjectExplorerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ProjectExplorer.json")

    friend class ProjectExplorerPluginPrivate;

public:
    ProjectExplorerPlugin();
    ~ProjectExplorerPlugin() override;

    static ProjectExplorerPlugin *instance();

    static OpenProjectResult openProject(const Utils::FilePath &filePath, bool searchInDir = true);
    static OpenProjectResult openProjects(const Utils::FilePaths &filePaths, bool searchInDir = true);
    static void showOpenProjectError(const OpenProjectResult &result);
    static void openProjectWelcomePage(const Utils::FilePath &filePath);
    static void unloadProject(Project *project);

    static bool saveModifiedFiles();

    static void showContextMenu(QWidget *view, const QPoint &globalPos, Node *node);

    //PluginInterface
    bool initialize(const QStringList &arguments, QString *errorMessage) override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;
    ShutdownFlag aboutToShutdown() override;

    static void setCustomParsers(const QList<CustomParserSettings> &settings);
    static void addCustomParser(const CustomParserSettings &settings);
    static void removeCustomParser(Utils::Id id);
    static const QList<CustomParserSettings> customParsers();

    static void startRunControl(RunControl *runControl);

    static Utils::FilePairs renameFiles(
        const QList<std::pair<Node *, Utils::FilePath>> &nodesAndNewFilePaths);

#ifdef WITH_TESTS
    static bool renameFile(const Utils::FilePath &source, const Utils::FilePath &target,
                           Project *project = nullptr);
#endif

    static QStringList projectFilePatterns();
    static bool isProjectFile(const Utils::FilePath &filePath);
    static RecentProjectsEntries recentProjects();

    static void renameFilesForSymbol(const QString &oldSymbolName, const QString &newSymbolName,
                                     const Utils::FilePaths &files, bool preferLowerCaseFileNames);

    static Utils::Result<> canRunStartupProject(Utils::Id runMode);
    static void runProject(Project *pro, Utils::Id, const bool forceSkipDeploy = false);
    static void runStartupProject(Utils::Id runMode, bool forceSkipDeploy = false);
    static void runRunConfiguration(RunConfiguration *rc, Utils::Id runMode,
                             const bool forceSkipDeploy = false);
    static QList<RunControl *> allRunControls();

    static void addExistingFiles(FolderNode *folderNode, const Utils::FilePaths &filePaths);

    static void initiateInlineRenaming();

    static QStringList projectFileGlobs();

    static QThreadPool *sharedThreadPool();
    static Internal::MiniProjectTargetSelector *targetSelector();

    static void openNewProjectDialog();
    static void openOpenProjectDialog();

    static void updateActions();

    static void activateProjectPanel(Utils::Id panelId);
    static void clearRecentProjects();
    static void removeFromRecentProjects(const Utils::FilePath &filePath);

    static void updateRunActions();
    static void updateVcsActions(const QString &vcsDisplayName);

    static Core::OutputWindow *buildSystemOutput();

signals:
    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged();

    void recentProjectsChanged();

    void settingsChanged();
    void customParsersChanged();

    void runActionsUpdated();
    void runControlStarted(ProjectExplorer::RunControl *runControl);
    void runControlStoped(ProjectExplorer::RunControl *runControl);

    void filesRenamed(const Utils::FilePairs &oldAndNewPaths);

private:
    static bool coreAboutToClose();
    void handleCommandLineArguments(const QStringList &arguments);
};

} // namespace ProjectExplorer
