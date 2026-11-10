// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeautogenparser.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakecodestyle.h"
#include "cmakebuildsystem.h"
#include "cmakeeditor.h"
#include "cmakefilecompletionassist.h"
#include "cmakeformatter.h"
#include "cmakeinstallstep.h"
#include "cmakelocatorfilter.h"
#include "cmakekitaspect.h"
#include "cmakeoutline.h"
#include "cmakeoutputparser.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectmanagertr.h"
#include "cmakequickfixes.h"
#include "cmakesemantichighlighter.h"
#include "cmakesettingspage.h"
#include "cmaketool.h"
#include "cmaketoolmanager.h"
#include "cmakeusages.h"
#include "conditionalsources.h"
#include "fileapidataextractor.h"
#include "mcptools.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <texteditor/snippets/snippetprovider.h>

#include <utils/mimeconstants.h>
#include <utils/fsengine/fileiconprovider.h>

#include <QTimer>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class CMakeToolAspectFactory : public DeviceToolAspectFactory
{
public:
    CMakeToolAspectFactory()
    {
        setToolId(Constants::CMAKE_TOOL_ID);
        setToolType(DeviceToolAspect::BuildTool);
        setFilePattern({"cmake",
                        "/Applications/CMake.app/Contents/bin/cmake",
                        "/usr/local/bin/cmake",
                        "/opt/homebrew/bin/cmake",
                        "/opt/local/bin/cmake",
                        "C:/Program Files/CMake/bin/cmake",
                        "C:/Program Files (x86)/CMake/bin/cmake"});
        setLabelText(Tr::tr("CMake executable:"));
        setDisplayName(Tr::tr("CMake"));
        const Id checkId("CMakeProjectManager.DeviceToolCheck");
        setChecker([checkId](const DeviceConstRef &device, const FilePath &candidate) -> Result<> {
            // Absolute file patterns are looked up on the host, whatever the device is.
            if (!candidate.isSameDevice(device.filePath("/")))
                return ResultError(Tr::tr("CMake executable is not on the device."));
            CMakeTool cmake(DetectionSource::FromSystem, checkId);
            cmake.setFilePath(candidate);
            if (!cmake.isValid()) {
                return ResultError(Tr::tr("CMake executable does not provide required IDE "
                                          "integration features."));
            }
            return ResultOk;
        });
    }
};

static bool activatePreset(Project *project, const QString &presetName)
{
    if (!qobject_cast<CMakeProject *>(project))
        return false;

    // Opening the project created a kit for each of its presets
    Kit *kit = KitManager::kit(CMakeConfigurationKitAspect::cmakePresetKitId(
        project->projectFilePath().toSettings().toString(), presetName));
    if (!kit)
        return false;

    Target *target = project->target(kit);
    if (!target)
        target = project->addTargetForKit(kit);
    if (!target)
        return false;
    project->setActiveTarget(target, SetActive::Cascade);
    return true;
}

static void activatePresetFromCommandLine(
    const QString &presetName, QList<Project *> projects, bool hasProjectArguments)
{
    if (!hasProjectArguments) {
        if (Project *project = ProjectManager::startupProject())
            projects.append(project);
    }

    bool activated = false;
    for (Project *project : std::as_const(projects))
        activated |= activatePreset(project, presetName);
    if (!activated) {
        MessageManager::writeFlashing(
            Tr::tr("The CMake preset \"%1\" from the command line was not activated.")
                .arg(presetName));
    }
}

static QString presetFromOptions(const QStringList &options)
{
    const int index = options.indexOf("-preset");
    if (index >= 0 && index + 1 < options.size())
        return options.at(index + 1);
    return {};
}

class CMakeProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CMakeProjectManager.json")

    // ProjectExplorer handles -project after this, both at startup and in
    // PluginManager::remoteArguments(), which goes by plugin id
    void activatePresetWhenProjectsOpen(const QStringList &options)
    {
        const QString presetName = presetFromOptions(options);
        if (presetName.isEmpty())
            return;
        connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::commandLineProjectsOpened,
                this, [presetName](const QList<Project *> &projects, bool hasProjectArguments) {
                    activatePresetFromCommandLine(presetName, projects, hasProjectArguments);
                }, Qt::SingleShotConnection);
    }

    Result<> initialize(const QStringList &arguments) final
    {
        activatePresetWhenProjectsOpen(arguments);
        initialize();
        return ResultOk;
    }

    QObject *remoteCommand(const QStringList &options, const QString &, const QStringList &) final
    {
        activatePresetWhenProjectsOpen(options);
        return nullptr;
    }

    void initialize() final
    {
        IOptionsPage::registerCategory(
            Constants::Settings::CATEGORY, Tr::tr("CMake"), Constants::Icons::SETTINGS_CATEGORY);

        setupCMakeToolManager(this);

        setupCMakeSettingsPage();
        setupCMakeKitAspects();

        setupCMakeBuildConfiguration();
        setupCMakeBuildStep();
        setupCMakeInstallStep();

        setupCMakeCodeStyle();
        setupCMakeEditor();
        setupCMakeOutline();

        setupCMakeLocatorFilters();
        setupCMakeFormatter();

        setupCMakeManager();

        registerMcpTools();

#ifdef WITH_TESTS
        addTestCreator(createCMakeConfigTest);
        addTestCreator(createCMakeOutputParserTest);
        addTestCreator(createCMakeAutogenParserTest);
        addTestCreator(createCMakeProjectImporterTest);
        addTestCreator(createCMakeQuickFixesTest);
        addTestCreator(createCMakeFunctionHintTest);
        addTestCreator(createCMakeSemanticHighlighterTest);
        addTestCreator(createAddDependenciesTest);
        addTestCreator(createBinariesForSourceFileTest);
        addTestCreator(createCMakeHelperDirectoryTest);
        addTestCreator(createCMakeKitAspectTest);
        addTestCreator(createImportedLibraryDirectoriesTest);
        addTestCreator(createJavaBuildTargetTest);
        addTestCreator(createJavaExecutableTest);
        addTestCreator(createJavaExtraDataTest);
        addTestCreator(createJavaSourcesTest);
        addTestCreator(createLinkedInterfaceSourcesTest);
        addTestCreator(createLibrarySearchPathsTest);
        addTestCreator(createConditionalSourcesTest);
        addTestCreator(createQmlModuleFilesTest);
        addTestCreator(createResourceFilesTest);
        addTestCreator(createSourceFilesTest);
        addTestCreator(createTargetSymbolsTest);
        addTestCreator(createTracepointFilesTest);
        addTestCreator(createCMakeUsagesTest);
        addTestCreator(createCMakeCodeStyleTest);
        addTestCreator(createTestPresetsInheritanceTest);
#endif

        FileIconProvider::registerIconOverlayForSuffix(Constants::Icons::FILE_OVERLAY, "cmake");
        FileIconProvider::registerIconOverlayForFilename(Constants::Icons::FILE_OVERLAY,
                                                         Constants::CMAKE_LISTS_TXT);

        TextEditor::SnippetProvider::registerGroup(Constants::CMAKE_SNIPPETS_GROUP_ID,
                                                   Tr::tr("CMake", "SnippetProvider"),
                                                   &decorateCMakeEditor);
        const auto issuesGenerator = [](const Kit *k) {
            Tasks result;
            if (CMakeKitAspect::cmakeExecutable(k).isEmpty()) {
                result.append(
                    Project::createTask(Task::TaskType::Error, Tr::tr("No cmake tool set.")));
            }
            if (ToolchainKitAspect::toolChains(k).isEmpty()) {
                result.append(
                    Project::createTask(
                        Task::TaskType::Warning, Tr::tr("No compilers set in kit.")));
            }
            return result;
        };
        ProjectManager::registerProjectType<CMakeProject>(
            Utils::Constants::CMAKE_PROJECT_MIMETYPE, issuesGenerator);
    }

    void extensionsInitialized() final
    {
        // Delay the restoration to allow the devices to load first.
        connect(DeviceManager::instance(), &DeviceManager::devicesLoaded, this, [] {
            CMakeToolManager::restoreCMakeTools();
        });

        setupOnlineHelpManager();
    }

    CMakeToolAspectFactory m_cmakeToolFactory;
};

} // CMakeProjectManager::Internal

#include "cmakeprojectplugin.moc"
