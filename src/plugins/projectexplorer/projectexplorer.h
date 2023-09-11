// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <extensionsystem/iplugin.h>

#include <utils/expected.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <QPair>

QT_BEGIN_NAMESPACE
class QPoint;
class QThreadPool;
QT_END_NAMESPACE

namespace Core {
class OutputWindow;
} // Core

namespace Utils { class CommandLine; }

namespace ProjectExplorer {
class CustomParserSettings;
class FolderNode;
class Node;
class Project;
class ProjectExplorerSettings;
class RunControl;
class RunConfiguration;

namespace Internal {
class AppOutputSettings;
class MiniProjectTargetSelector;
}

using RecentProjectsEntry = QPair<Utils::FilePath, QString>;
using RecentProjectsEntries = QList<RecentProjectsEntry>;

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

    static OpenProjectResult openProject(const Utils::FilePath &filePath);
    static OpenProjectResult openProjects(const Utils::FilePaths &filePaths);
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

    static void setProjectExplorerSettings(const ProjectExplorerSettings &pes);
    static const ProjectExplorerSettings &projectExplorerSettings();

    static void setAppOutputSettings(const Internal::AppOutputSettings &settings);
    static const Internal::AppOutputSettings &appOutputSettings();

    static void setCustomParsers(const QList<CustomParserSettings> &settings);
    static void addCustomParser(const CustomParserSettings &settings);
    static void removeCustomParser(Utils::Id id);
    static const QList<CustomParserSettings> customParsers();

    static void startRunControl(RunControl *runControl);
    static void showOutputPaneForRunControl(RunControl *runControl);

    // internal public for FlatModel
    static void renameFile(Node *node, const QString &newFilePath);
    static QStringList projectFilePatterns();
    static bool isProjectFile(const Utils::FilePath &filePath);
    static RecentProjectsEntries recentProjects();

    static void renameFilesForSymbol(const QString &oldSymbolName, const QString &newSymbolName,
                                     const Utils::FilePaths &files, bool preferLowerCaseFileNames);

    static Utils::expected_str<void> canRunStartupProject(Utils::Id runMode);
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

    static Core::OutputWindow *buildSystemOutput();

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
    void testProject_multipleBuildConfigs();

    void testSourceToBinaryMapping();
    void testSourceToBinaryMapping_data();

    void testSessionSwitch();
#endif // WITH_TESTS
};

} // namespace ProjectExplorer
