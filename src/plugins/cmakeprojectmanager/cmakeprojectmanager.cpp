// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectmanager.h"

#include "cmakebuildsystem.h"
#include "cmakebuildconfiguration.h"
#include "cmakekitaspect.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeprojectnodes.h"
#include "cmakespecificsettings.h"
#include "cmaketoolmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>

#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <utils/checkablemessagebox.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QMessageBox>

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class CMakeManager final : public QObject
{
public:
    CMakeManager();

    static bool isCMakeUrl(const QUrl &url);
    static void openCMakeUrl(const QUrl &url);
    static void runSubprojectOperation(
        BuildSystem *bs, const ProjectNode *node, const QString &clean, const QString &build);

private:
    void updateCMakeActions();
    void updateCMakeBuildTarget(Node *node);
    void clearCMakeCache(BuildSystem *buildSystem);
    void rescanProject(BuildSystem *buildSystem);
    void reloadCMakePresets();
    void runSubprojectOperation(const QString &clean, const QString &build);

    QAction *m_runCMakeAction;
    QAction *m_clearCMakeCacheAction;
    QAction *m_runCMakeActionContextMenu;
    QAction *m_clearCMakeCacheActionContextMenu;
    QAction *m_rescanProjectAction;
    QAction *m_reloadCMakePresetsAction;
    QAction *m_cmakeProfilerAction;
    QAction *m_cmakeDebuggerAction;
    QAction *m_cmakeDebuggerSeparator;
    bool m_canDebugCMake = false;
};

bool CMakeManager::isCMakeUrl(const QUrl &url)
{
    const QString address = url.toString();
    return address.startsWith("qthelp://org.cmake.");
}

void CMakeManager::openCMakeUrl(const QUrl &url)
{
    QString urlPrefix = "https://cmake.org/cmake/help/";

    static const QRegularExpression version("^.*\\.([0-9])\\.([0-9]+)\\.[0-9]+$");
    auto match = version.match(url.authority());
    if (match.hasMatch())
        urlPrefix.append(QString("v%1.%2").arg(match.captured(1)).arg(match.captured(2)));
    else
        urlPrefix.append("latest");

    const QString address = url.toString();
    const QString doc("/doc");
    QDesktopServices::openUrl(
        QUrl(urlPrefix + address.mid(address.lastIndexOf(doc) + doc.size())));
}

CMakeManager::CMakeManager()
{
    namespace PEC = ProjectExplorer::Constants;

    const Context projectContext(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);

    ActionBuilder(this, Constants::RUN_CMAKE)
        .setText(Tr::tr("Run CMake"))
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .bindContextAction(&m_runCMakeAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [] { runCMake(activeBuildSystemForActiveProject()); });

    ActionBuilder(this, Constants::CLEAR_CMAKE_CACHE)
        .setText(Tr::tr("Clear CMake Configuration"))
        .bindContextAction(&m_clearCMakeCacheAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] { clearCMakeCache(activeBuildSystemForActiveProject()); });

    ActionBuilder(this, Constants::RUN_CMAKE_CONTEXT_MENU)
        .setText(Tr::tr("Run CMake"))
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .setContext(projectContext)
        .bindContextAction(&m_runCMakeActionContextMenu)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_BUILD)
        .addOnTriggered(this, [] { runCMake(activeBuildSystemForCurrentProject()); });

    ActionBuilder(this, Constants::CLEAR_CMAKE_CACHE_CONTEXT_MENU)
        .setText(Tr::tr("Clear CMake Configuration"))
        .setContext(projectContext)
        .bindContextAction(&m_clearCMakeCacheActionContextMenu)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_REBUILD)
        .addOnTriggered(this, [this] { clearCMakeCache(activeBuildSystemForCurrentProject()); });

    ActionBuilder(this, Constants::RESCAN_PROJECT)
        .setText(Tr::tr("Rescan Project"))
        .bindContextAction(&m_rescanProjectAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] { rescanProject(activeBuildSystemForCurrentProject()); });

    ActionBuilder(this, Constants::RELOAD_CMAKE_PRESETS)
        .setText(Tr::tr("Reload CMake Presets"))
        .setIcon(Utils::Icons::RELOAD.icon())
        .bindContextAction(&m_reloadCMakePresetsAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] { reloadCMakePresets(); });

    // CMake Profiler
    ActionBuilder(this, Constants::RUN_CMAKE_PROFILER)
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .setText(Tr::tr("CMake Profiler"))
        .bindContextAction(&m_cmakeProfilerAction)
        .addToContainer(Core::Constants::M_DEBUG_ANALYZER, Core::Constants::G_ANALYZER_TOOLS, false)
        .addOnTriggered(this, [] { runCMakeWithProfiling(activeBuildSystemForActiveProject()); });

    // CMake Debugger
    ActionContainer *mdebugger = ActionManager::actionContainer(PEC::M_DEBUG_STARTDEBUGGING);
    mdebugger->appendGroup(Constants::CMAKE_DEBUGGING_GROUP);
    mdebugger->addSeparator(Context(Core::Constants::C_GLOBAL),
                            Constants::CMAKE_DEBUGGING_GROUP,
                            &m_cmakeDebuggerSeparator);

    ActionBuilder(this, Constants::RUN_CMAKE_DEBUGGER)
        .setText(Tr::tr("Start CMake Debugging"))
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .bindContextAction(&m_cmakeDebuggerAction)
        .addToContainer(PEC::M_DEBUG_STARTDEBUGGING, Constants::CMAKE_DEBUGGING_GROUP)
        .addOnTriggered(this, [] {
            ProjectExplorerPlugin::runStartupProject(PEC::DAP_CMAKE_DEBUG_RUN_MODE,
                                                     /*forceSkipDeploy=*/true);
        });

    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged, this, [this] {
        if (BuildSystem *buildSystem = activeBuildSystemForActiveProject()) {
            FilePath cmakeExecutable = CMakeKitAspect::cmakeExecutable(buildSystem->kit());
            const CMakeTool *tool = CMakeToolManager::findByCommand(cmakeExecutable);
            CMakeTool::Version version = tool ? tool->version() : CMakeTool::Version();
            m_canDebugCMake = (version.major == 3 && version.minor >= 27) || version.major > 3;
        }
        updateCMakeActions();
    });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this, [this] {
        updateCMakeActions();
    });
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged, this, [this](Node *node) {
        updateCMakeActions();
        updateCMakeBuildTarget(node);
    });

    updateCMakeActions();
}

void CMakeManager::updateCMakeActions()
{
    auto project = qobject_cast<CMakeProject *>(ProjectManager::startupProject());
    const bool visible = project && !BuildManager::isBuilding(project);
    m_runCMakeAction->setVisible(visible);
    m_runCMakeActionContextMenu->setEnabled(visible);
    m_clearCMakeCacheActionContextMenu->setVisible(visible);
    m_clearCMakeCacheAction->setVisible(visible);
    m_rescanProjectAction->setVisible(visible);
    m_cmakeProfilerAction->setEnabled(visible);

    m_cmakeDebuggerAction->setEnabled(m_canDebugCMake && visible);
    m_cmakeDebuggerSeparator->setVisible(m_canDebugCMake && visible);

    const bool reloadPresetsVisible = [project] {
        if (!project)
            return false;
        const FilePath presetsPath = project->projectFilePath().parentDir().pathAppended(
            "CMakePresets.json");
        return presetsPath.exists();
    }();
    m_reloadCMakePresetsAction->setVisible(reloadPresetsVisible);
}

void CMakeManager::updateCMakeBuildTarget(Node *node)
{
    if (!node)
        return;

    auto bs = qobject_cast<CMakeBuildSystem *>(activeBuildSystemForCurrentProject());
    if (!bs)
        return;

    auto targetNode = dynamic_cast<const CMakeTargetNode *>(node);

    bs->cmakeBuildConfiguration()->setRestrictedBuildTarget(
        targetNode ? targetNode->buildKey() : QString());
}

void CMakeManager::clearCMakeCache(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return);

    cmakeBuildSystem->clearCMakeCache();
    cmakeBuildSystem->disableCMakeBuildMenuActions();
}

void runCMake(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return );

    if (ProjectExplorerPlugin::saveModifiedFiles())
        cmakeBuildSystem->runCMake();
}

void runCMakeWithProfiling(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return );

    if (ProjectExplorerPlugin::saveModifiedFiles()) {
        // cmakeBuildSystem->runCMakeWithProfiling() below will trigger Target::buildSystemUpdated
        // which will ensure that the "cmake-profile.json" has been created and we can load the viewer
        QObject::connect(cmakeBuildSystem, &BuildSystem::updated, cmakeBuildSystem, [] {
            Core::Command *ctfVisualiserLoadTrace = Core::ActionManager::command(
                "Analyzer.Menu.StartAnalyzer.CtfVisualizer.LoadTrace");

            if (ctfVisualiserLoadTrace) {
                auto *action = ctfVisualiserLoadTrace->actionForContext(Core::Constants::C_GLOBAL);
                const FilePath file = TemporaryDirectory::masterDirectoryFilePath()
                                      / "cmake-profile.json";
                action->setData(file.nativePath());
                emit ctfVisualiserLoadTrace->action()->triggered();
            }
        }, Qt::SingleShotConnection);

        cmakeBuildSystem->runCMakeWithProfiling();
    }
}
void runSubprojectOperation(
    ProjectExplorer::BuildSystem *bs,
    const ProjectExplorer::ProjectNode *node,
    ProjectExplorer::BuildAction action)
{
    switch (action) {
    case BuildAction::Build:
        CMakeManager::runSubprojectOperation(bs, node, {}, "all");
        break;
    case BuildAction::Clean:
        CMakeManager::runSubprojectOperation(bs, node, "clean", {});
        break;
    case BuildAction::Rebuild:
        CMakeManager::runSubprojectOperation(bs, node, "clean", "all");
        break;
    }
}

void CMakeManager::rescanProject(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return);

    cmakeBuildSystem->runCMakeAndScanProjectTree();// by my experience: every rescan run requires cmake run too
}

void CMakeManager::reloadCMakePresets()
{
    CMakeProject *project = qobject_cast<CMakeProject *>(ProjectTree::currentProject());
    if (!project)
        return;

    QMessageBox::StandardButton clickedButton = CheckableMessageBox::question(
        Tr::tr("Reload CMake Presets"),
        Tr::tr("Re-generates the kits that were created for CMake presets. All manual "
               "modifications to the CMake project settings will be lost."),
        cmakeSettingsForProject(project).askBeforePresetsReload.askAgainCheckableDecider(),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Yes,
        QMessageBox::Yes,
        {
            {QMessageBox::Yes, Tr::tr("Reload")},
        });

    cmakeSettingsForProject(project).writeSettings();

    if (clickedButton == QMessageBox::Cancel)
        return;

    const auto presetKitId = [project](const QString &presetName) {
        return CMakeConfigurationKitAspect::cmakePresetKitId(
            project->projectFilePath().path(), presetName);
    };

    const QSet<Id> oldPresets = Utils::transform<QSet>(
        project->presetsData().configurePresets,
        [presetKitId](const auto &preset) { return presetKitId(preset.name); });

    QList<Kit *> oldKits
        = Utils::filtered(KitManager::kits(), [presetKitId, oldPresets](const Kit *k) {
              const auto presetConfigItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
              return !presetConfigItem.isNull()
                     && oldPresets.contains(presetKitId(QString::fromUtf8(presetConfigItem.value)));
          });

    for (const auto &target : project->targets()) {
        const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(
            target->kit());

        if (BuildManager::isBuilding(target))
            BuildManager::cancel();

        // Only clear the CMake configuration for preset kits. Any manual kit configuration
        // will get the chance to get imported afterwards in the Kit selection wizard
        CMakeBuildSystem *bs = static_cast<CMakeBuildSystem *>(target->buildSystem());
        if (!presetItem.isNull() && bs)
            bs->clearCMakeCache();

        project->removeTarget(target);
    }

    KitManager::deregisterKits(oldKits);

    project->readPresets();
    project->createKitsFromPresets();

    // Switch to configure the build configurations for the Kits
    Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    Core::ModeManager::setFocusToCurrentMode();
}

void CMakeManager::runSubprojectOperation(
    BuildSystem *bs, const ProjectNode *node, const QString &clean, const QString &build)
{
    const auto cmakeBs = qobject_cast<CMakeBuildSystem *>(bs);
    QTC_ASSERT(cmakeBs, return);

    auto subProjectDir = node->filePath();
    auto projectFileDir = bs->project()->projectFilePath().parentDir();
    auto relativePath = subProjectDir.relativeChildPath(projectFileDir);
    if (clean.isEmpty()) {
        cmakeBs->buildCMakeTarget(relativePath.path() + "/" + build);
    } else if (build.isEmpty()) {
        cmakeBs->buildCMakeTarget(relativePath.path() + "/" + clean);
    } else {
        cmakeBs->reBuildCMakeTarget(
            relativePath.path() + "/" + clean, relativePath.path() + "/" + build);
    }
}

void setupCMakeManager()
{
    static CMakeManager theCMakeManager;
}

void setupOnlineHelpManager()
{
    Core::HelpManager::addOnlineHelpHandler({CMakeManager::isCMakeUrl, CMakeManager::openCMakeUrl});
}

} // CMakeProjectManager::Internal
