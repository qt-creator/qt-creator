/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_export.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"

#include <extensionsystem/iplugin.h>

#include <QPair>

QT_BEGIN_NAMESPACE
class QPoint;
class QAction;
class QThreadPool;
QT_END_NAMESPACE

namespace Core {
class IMode;
class Id;
} // namespace Core

namespace Utils { class ProcessHandle; }

namespace ProjectExplorer {
class RunControl;
class RunConfiguration;
class IRunControlFactory;
class Project;
class Node;
class FolderNode;
class FileNode;

namespace Internal { class ProjectExplorerSettings; }

class PROJECTEXPLORER_EXPORT ProjectExplorerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ProjectExplorer.json")

    friend class ProjectExplorerPluginPrivate;

public:
    ProjectExplorerPlugin();
    ~ProjectExplorerPlugin() override;

    static ProjectExplorerPlugin *instance();

    class OpenProjectResult
    {
    public:
        OpenProjectResult(QList<Project *> projects, QList<Project *> alreadyOpen,
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

    static OpenProjectResult openProject(const QString &fileName);
    static OpenProjectResult openProjects(const QStringList &fileNames);
    static void showOpenProjectError(const OpenProjectResult &result);
    static void openProjectWelcomePage(const QString &fileName);
    static void unloadProject(Project *project);

    static bool saveModifiedFiles();

    static void showContextMenu(QWidget *view, const QPoint &globalPos, Node *node);

    //PluginInterface
    bool initialize(const QStringList &arguments, QString *errorMessage) override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;
    ShutdownFlag aboutToShutdown() override;

    static void setProjectExplorerSettings(const Internal::ProjectExplorerSettings &pes);
    static Internal::ProjectExplorerSettings projectExplorerSettings();

    static void startRunControl(RunControl *runControl, Core::Id runMode);
    static void showRunErrorMessage(const QString &errorMessage);

    // internal public for FlatModel
    static void renameFile(Node *node, const QString &newFilePath);
    static QStringList projectFilePatterns();
    static QList<QPair<QString, QString> > recentProjects();

    static bool canRunStartupProject(Core::Id runMode, QString *whyNot = nullptr);
    static void runProject(Project *pro, Core::Id, const bool forceSkipDeploy = false);
    static void runStartupProject(Core::Id runMode, bool forceSkipDeploy = false);
    static void runRunConfiguration(RunConfiguration *rc, Core::Id runMode,
                             const bool forceSkipDeploy = false);
    static QList<QPair<Runnable, Utils::ProcessHandle>> runningRunControlProcesses();

    static void addExistingFiles(FolderNode *folderNode, const QStringList &filePaths);

    static void buildProject(Project *p);

    static void initiateInlineRenaming();

    static QString displayNameForStepId(Core::Id stepId);

    static QString directoryFor(Node *node);
    static QStringList projectFileGlobs();

    static void updateContextMenuActions();

    static QThreadPool *sharedThreadPool();

    static void openNewProjectDialog();
    static void openOpenProjectDialog();

signals:
    void finishedInitialization();
    void runControlStarted(ProjectExplorer::RunControl *rc);
    void runControlFinished(ProjectExplorer::RunControl *rc);

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged();

    void aboutToExecuteProject(ProjectExplorer::Project *project, Core::Id runMode);
    void recentProjectsChanged();

    void settingsChanged();

    void updateRunActions();

private:
    static bool coreAboutToClose();

#ifdef WITH_TESTS
private slots:
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

    void testClangClOutputParsers_data();
    void testClangClOutputParsers();

    void testGccAbiGuessing_data();
    void testGccAbiGuessing();

    void testAbiOfBinary_data();
    void testAbiOfBinary();
    void testFlavorForOs();
    void testAbiFromTargetTriplet_data();
    void testAbiFromTargetTriplet();

    void testDeviceManager();

    void testToolChainManager_data();
    void testToolChainManager();
#endif // WITH_TESTS
};

} // namespace ProjectExplorer
