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
} // namespace Core

namespace Utils {
class ProcessHandle;
class FilePath;
}

namespace ProjectExplorer {
class BuildPropertiesSettings;
class RunControl;
class RunConfiguration;
class Project;
class Node;
class FolderNode;
class FileNode;

namespace Internal {
class AppOutputSettings;
class CustomParserSettings;
class MiniProjectTargetSelector;
class ProjectExplorerSettings;
}

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
    void restoreKits();
    ShutdownFlag aboutToShutdown() override;

    static void setProjectExplorerSettings(const Internal::ProjectExplorerSettings &pes);
    static const Internal::ProjectExplorerSettings &projectExplorerSettings();

    static void setAppOutputSettings(const Internal::AppOutputSettings &settings);
    static const Internal::AppOutputSettings &appOutputSettings();

    static void setBuildPropertiesSettings(const BuildPropertiesSettings &settings);
    static const BuildPropertiesSettings &buildPropertiesSettings();
    static void showQtSettings();

    static void setCustomParsers(const QList<Internal::CustomParserSettings> &settings);
    static const QList<Internal::CustomParserSettings> customParsers();

    static void startRunControl(RunControl *runControl);
    static void showOutputPaneForRunControl(RunControl *runControl);

    // internal public for FlatModel
    static void renameFile(Node *node, const QString &newFilePath);
    static QStringList projectFilePatterns();
    static bool isProjectFile(const Utils::FilePath &filePath);
    static QList<QPair<QString, QString> > recentProjects();

    static bool canRunStartupProject(Utils::Id runMode, QString *whyNot = nullptr);
    static void runProject(Project *pro, Utils::Id, const bool forceSkipDeploy = false);
    static void runStartupProject(Utils::Id runMode, bool forceSkipDeploy = false);
    static void runRunConfiguration(RunConfiguration *rc, Utils::Id runMode,
                             const bool forceSkipDeploy = false);
    static QList<QPair<Runnable, Utils::ProcessHandle>> runningRunControlProcesses();
    static QList<RunControl *> allRunControls();

    static void addExistingFiles(FolderNode *folderNode, const QStringList &filePaths);

    static void initiateInlineRenaming();

    static QStringList projectFileGlobs();

    static QThreadPool *sharedThreadPool();
    static Internal::MiniProjectTargetSelector *targetSelector();

    static void showSessionManager();
    static void openNewProjectDialog();
    static void openOpenProjectDialog();

    static QString buildDirectoryTemplate();
    static QString defaultBuildDirectoryTemplate();

    static void updateActions();

    static void activateProjectPanel(Utils::Id panelId);
    static void clearRecentProjects();
    static void removeFromRecentProjects(const QString &fileName, const QString &displayName);

    static void updateRunActions();

signals:
    void finishedInitialization();

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged();

    void recentProjectsChanged();

    void settingsChanged();
    void customParsersChanged();

    void runActionsUpdated();

private:
    static bool coreAboutToClose();
    void handleCommandLineArguments(const QStringList &arguments);

#ifdef WITH_TESTS
private slots:
    void testJsonWizardsEmptyWizard();
    void testJsonWizardsEmptyPage();
    void testJsonWizardsUnusedKeyAtFields_data();
    void testJsonWizardsUnusedKeyAtFields();
    void testJsonWizardsCheckBox();
    void testJsonWizardsLineEdit();
    void testJsonWizardsComboBox();
    void testJsonWizardsIconList();

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
    void testGnuMakeParserTaskMangling();

    void testXcodebuildParserParsing_data();
    void testXcodebuildParserParsing();

    void testMsvcOutputParsers_data();
    void testMsvcOutputParsers();

    void testClangClOutputParsers_data();
    void testClangClOutputParsers();

    void testGccAbiGuessing_data();
    void testGccAbiGuessing();

    void testAbiRoundTrips();
    void testAbiOfBinary_data();
    void testAbiOfBinary();
    void testAbiFromTargetTriplet_data();
    void testAbiFromTargetTriplet();
    void testAbiUserOsFlavor_data();
    void testAbiUserOsFlavor();

    void testDeviceManager();

    void testToolChainMerging_data();
    void testToolChainMerging();
    void deleteTestToolchains();

    void testUserFileAccessor_prepareToReadSettings();
    void testUserFileAccessor_prepareToReadSettingsObsoleteVersion();
    void testUserFileAccessor_prepareToReadSettingsObsoleteVersionNewVersion();
    void testUserFileAccessor_prepareToWriteSettings();
    void testUserFileAccessor_mergeSettings();
    void testUserFileAccessor_mergeSettingsEmptyUser();
    void testUserFileAccessor_mergeSettingsEmptyShared();

    void testProject_setup();
    void testProject_changeDisplayName();
    void testProject_parsingSuccess();
    void testProject_parsingFail();
    void testProject_projectTree();

    void testSessionSwitch();
#endif // WITH_TESTS
};

} // namespace ProjectExplorer
