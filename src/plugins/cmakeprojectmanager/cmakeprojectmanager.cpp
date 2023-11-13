// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectmanager.h"

#include "cmakebuildsystem.h"
#include "cmakekitaspect.h"
#include "cmakeprocess.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeprojectnodes.h"
#include "cmakespecificsettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>

#include <cppeditor/cpptoolsreuse.h>

#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/checkablemessagebox.h>
#include <utils/utilsicons.h>
#include <utils/parameteraction.h>

#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

CMakeManager::CMakeManager()
{
    namespace PEC = ProjectExplorer::Constants;

    const Context projectContext(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);

    ActionBuilder runCMakeAction(this, Constants::RUN_CMAKE);
    runCMakeAction.setText(Tr::tr("Run CMake"));
    runCMakeAction.setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon());
    runCMakeAction.bindContextAction(&m_runCMakeAction);
    runCMakeAction.setCommandAttribute(Command::CA_Hide);
    runCMakeAction.setContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD);
    runCMakeAction.setOnTriggered(this, [this] { runCMake(ProjectManager::startupBuildSystem()); });

    ActionBuilder clearCMakeCacheAction(this, Constants::CLEAR_CMAKE_CACHE);
    clearCMakeCacheAction.setText(Tr::tr("Clear CMake Configuration"));
    clearCMakeCacheAction.bindContextAction(&m_clearCMakeCacheAction);
    clearCMakeCacheAction.setCommandAttribute(Command::CA_Hide);
    clearCMakeCacheAction.setContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD);
    clearCMakeCacheAction.setOnTriggered(this, [this] {
        clearCMakeCache(ProjectManager::startupBuildSystem());
    });

    ActionBuilder runCMakeActionContextMenu(this, Constants::RUN_CMAKE_CONTEXT_MENU);
    runCMakeActionContextMenu.setText(Tr::tr("Run CMake"));
    runCMakeActionContextMenu.setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon());
    runCMakeActionContextMenu.setContext(projectContext);
    runCMakeActionContextMenu.bindContextAction(&m_runCMakeActionContextMenu);
    runCMakeActionContextMenu.setCommandAttribute(Command::CA_Hide);
    runCMakeActionContextMenu.setContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_BUILD);
    runCMakeActionContextMenu.setOnTriggered(this, [this] {
        runCMake(ProjectTree::currentBuildSystem());
    });

    ActionBuilder buildFileContextAction(this, Constants::BUILD_FILE_CONTEXT_MENU);
    buildFileContextAction.setText(Tr::tr("Build"));
    buildFileContextAction.bindContextAction(&m_buildFileContextMenu);
    buildFileContextAction.setContext(projectContext);
    buildFileContextAction.setCommandAttribute(Command::CA_Hide);
    buildFileContextAction.setContainer(PEC::M_FILECONTEXT, PEC::G_FILE_OTHER);
    buildFileContextAction.setOnTriggered(this, [this] { buildFileContextMenu(); });

    ActionBuilder rescanProjectAction(this, Constants::RESCAN_PROJECT);
    rescanProjectAction.setText(Tr::tr("Rescan Project"));
    rescanProjectAction.bindContextAction(&m_rescanProjectAction);
    rescanProjectAction.setCommandAttribute(Command::CA_Hide);
    rescanProjectAction.setContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD);
    rescanProjectAction.setOnTriggered(this, [this] {
        rescanProject(ProjectTree::currentBuildSystem());
    });

    ActionBuilder reloadCMakePresetsAction(this, Constants::RELOAD_CMAKE_PRESETS);
    reloadCMakePresetsAction.setText(Tr::tr("Reload CMake Presets"));
    reloadCMakePresetsAction.setIcon(Utils::Icons::RELOAD.icon());
    reloadCMakePresetsAction.bindContextAction(&m_reloadCMakePresetsAction);
    reloadCMakePresetsAction.setCommandAttribute(Command::CA_Hide);
    reloadCMakePresetsAction.setContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD);
    reloadCMakePresetsAction.setOnTriggered(this, [this] { reloadCMakePresets(); });

    m_buildFileAction = new Utils::ParameterAction(Tr::tr("Build File"),
                                                   Tr::tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled,
                                                   this);
    Command *command = ActionManager::registerAction(m_buildFileAction, Constants::BUILD_FILE);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+B")));
    ActionContainer *mbuild = ActionManager::actionContainer(PEC::M_BUILDPROJECT);
    mbuild->addAction(command, PEC::G_BUILD_BUILD);
    connect(m_buildFileAction, &QAction::triggered, this, [this] { buildFile(); });

    // CMake Profiler
    ActionBuilder cmakeProfilerAction(this, Constants::RUN_CMAKE_PROFILER);
    cmakeProfilerAction.setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon());
    cmakeProfilerAction.setText(Tr::tr("CMake Profiler"));
    cmakeProfilerAction.bindContextAction(&m_cmakeProfilerAction);
    cmakeProfilerAction.setCommandDescription(m_cmakeProfilerAction->text());
    cmakeProfilerAction.setContainer(Debugger::Constants::M_DEBUG_ANALYZER,
                                     Debugger::Constants::G_ANALYZER_TOOLS,
                                     false);
    cmakeProfilerAction.setOnTriggered(this, [this] {
        runCMakeWithProfiling(ProjectManager::startupBuildSystem());
    });

    // CMake Debugger
    ActionContainer *mdebugger = ActionManager::actionContainer(PEC::M_DEBUG_STARTDEBUGGING);
    mdebugger->appendGroup(Constants::CMAKE_DEBUGGING_GROUP);
    mdebugger->addSeparator(Context(Core::Constants::C_GLOBAL),
                            Constants::CMAKE_DEBUGGING_GROUP,
                            &m_cmakeDebuggerSeparator);

    ActionBuilder cmakeDebuggerAction(this, Constants::RUN_CMAKE_DEBUGGER);
    cmakeDebuggerAction.setText(Tr::tr("Start CMake Debugging"));
    cmakeDebuggerAction.setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon());
    cmakeDebuggerAction.bindContextAction(&m_cmakeDebuggerAction);
    cmakeDebuggerAction.setCommandDescription(m_cmakeDebuggerAction->text());
    cmakeDebuggerAction.setContainer(PEC::M_DEBUG_STARTDEBUGGING, Constants::CMAKE_DEBUGGING_GROUP);
    cmakeDebuggerAction.setOnTriggered(this, [] {
        ProjectExplorerPlugin::runStartupProject(PEC::DAP_CMAKE_DEBUG_RUN_MODE, false);
    });

    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged, this, [this] {
        auto cmakeBuildSystem = qobject_cast<CMakeBuildSystem *>(
            ProjectManager::startupBuildSystem());
        if (cmakeBuildSystem) {
            const BuildDirParameters parameters(cmakeBuildSystem);
            const auto tool = parameters.cmakeTool();
            CMakeTool::Version version = tool ? tool->version() : CMakeTool::Version();
            m_canDebugCMake = (version.major == 3 && version.minor >= 27) || version.major > 3;
        }
        updateCmakeActions(ProjectTree::currentNode());
    });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this, [this] {
        updateCmakeActions(ProjectTree::currentNode());
    });
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &CMakeManager::updateBuildFileAction);
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &CMakeManager::updateCmakeActions);

    updateCmakeActions(ProjectTree::currentNode());
}

void CMakeManager::updateCmakeActions(Node *node)
{
    auto project = qobject_cast<CMakeProject *>(ProjectManager::startupProject());
    const bool visible = project && !BuildManager::isBuilding(project);
    m_runCMakeAction->setVisible(visible);
    m_runCMakeActionContextMenu->setEnabled(visible);
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

    enableBuildFileMenus(node);
}

void CMakeManager::clearCMakeCache(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return);

    cmakeBuildSystem->clearCMakeCache();
}

void CMakeManager::runCMake(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return );

    if (ProjectExplorerPlugin::saveModifiedFiles())
        cmakeBuildSystem->runCMake();
}

void CMakeManager::runCMakeWithProfiling(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return );

    if (ProjectExplorerPlugin::saveModifiedFiles()) {
        // cmakeBuildSystem->runCMakeWithProfiling() below will trigger Target::buildSystemUpdated
        // which will ensure that the "cmake-profile.json" has been created and we can load the viewer
        std::unique_ptr<QObject> context{new QObject};
        QObject *pcontext = context.get();
        QObject::connect(cmakeBuildSystem->target(),
                         &Target::buildSystemUpdated,
                         pcontext,
                         [context = std::move(context)]() mutable {
                             context.reset();
                             Core::Command *ctfVisualiserLoadTrace = Core::ActionManager::command(
                                 "Analyzer.Menu.StartAnalyzer.CtfVisualizer.LoadTrace");

                             if (ctfVisualiserLoadTrace) {
                                 auto *action = ctfVisualiserLoadTrace->actionForContext(
                                     Core::Constants::C_GLOBAL);
                                 const FilePath file = TemporaryDirectory::masterDirectoryFilePath()
                                                       / "cmake-profile.json";
                                 action->setData(file.nativePath());
                                 emit ctfVisualiserLoadTrace->action()->triggered();
                             }
                         });

        cmakeBuildSystem->runCMakeWithProfiling();
    }
}

void CMakeManager::rescanProject(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return);

    cmakeBuildSystem->runCMakeAndScanProjectTree();// by my experience: every rescan run requires cmake run too
}

void CMakeManager::updateBuildFileAction()
{
    Node *node = nullptr;
    if (Core::IDocument *currentDocument = Core::EditorManager::currentDocument())
        node = ProjectTree::nodeForFile(currentDocument->filePath());
    enableBuildFileMenus(node);
}

void CMakeManager::enableBuildFileMenus(Node *node)
{
    m_buildFileAction->setVisible(false);
    m_buildFileAction->setEnabled(false);
    m_buildFileAction->setParameter(QString());
    m_buildFileContextMenu->setEnabled(false);

    if (!node)
        return;
    Project *project = ProjectTree::projectForNode(node);
    if (!project)
        return;
    Target *target = project->activeTarget();
    if (!target)
        return;
    const QString generator = CMakeGeneratorKitAspect::generator(target->kit());
    if (generator != "Ninja" && !generator.contains("Makefiles"))
        return;

    if (const FileNode *fileNode = node->asFileNode()) {
        const FileType type = fileNode->fileType();
        const bool visible = qobject_cast<CMakeProject *>(project)
                && dynamic_cast<CMakeTargetNode *>(node->parentProjectNode())
                && (type == FileType::Source || type == FileType::Header);

        const bool enabled = visible && !BuildManager::isBuilding(project);
        m_buildFileAction->setVisible(visible);
        m_buildFileAction->setEnabled(enabled);
        m_buildFileAction->setParameter(node->filePath().fileName());
        m_buildFileContextMenu->setEnabled(enabled);
    }
}

void CMakeManager::reloadCMakePresets()
{
    QMessageBox::StandardButton clickedButton = CheckableMessageBox::question(
        Core::ICore::dialogParent(),
        Tr::tr("Reload CMake Presets"),
        Tr::tr("Re-generates the kits that were created for CMake presets. All manual "
               "modifications to the CMake project settings will be lost."),
        settings().askBeforePresetsReload.askAgainCheckableDecider(),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Yes,
        QMessageBox::Yes,
        {
            {QMessageBox::Yes, Tr::tr("Reload")},
        });

    settings().writeSettings();

    if (clickedButton == QMessageBox::Cancel)
        return;

    CMakeProject *project = static_cast<CMakeProject *>(ProjectTree::currentProject());
    if (!project)
        return;

    const QSet<QString> oldPresets = Utils::transform<QSet>(project->presetsData().configurePresets,
                                                            [](const auto &preset) {
                                                                return preset.name;
                                                            });
    project->readPresets();

    QList<Kit*> oldKits;
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

        if (!presetItem.isNull() && oldPresets.contains(QString::fromUtf8(presetItem.value)))
            oldKits << target->kit();

        project->removeTarget(target);
    }

    project->setOldPresetKits(oldKits);

    Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    Core::ModeManager::setFocusToCurrentMode();
}

void CMakeManager::buildFile(Node *node)
{
    if (!node) {
        Core::IDocument *currentDocument= Core::EditorManager::currentDocument();
        if (!currentDocument)
            return;
        const Utils::FilePath file = currentDocument->filePath();
        node = ProjectTree::nodeForFile(file);
    }
    FileNode *fileNode  = node ? node->asFileNode() : nullptr;
    if (!fileNode)
        return;
    Project *project = ProjectTree::projectForNode(fileNode);
    if (!project)
        return;
    CMakeTargetNode *targetNode = dynamic_cast<CMakeTargetNode *>(fileNode->parentProjectNode());
    if (!targetNode)
        return;
    FilePath filePath = fileNode->filePath();
    if (filePath.fileName().contains(".h")) {
        bool wasHeader = false;
        const FilePath sourceFile = CppEditor::correspondingHeaderOrSource(filePath, &wasHeader);
        if (wasHeader && !sourceFile.isEmpty())
            filePath = sourceFile;
    }
    Target *target = project->activeTarget();
    QTC_ASSERT(target, return);
    const QString generator = CMakeGeneratorKitAspect::generator(target->kit());
    const QString relativeSource = filePath.relativeChildPath(targetNode->filePath()).toString();
    Utils::FilePath targetBase;
    BuildConfiguration *bc = target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    if (generator == "Ninja") {
        const Utils::FilePath relativeBuildDir = targetNode->buildDirectory().relativeChildPath(
                    bc->buildDirectory());
        targetBase = relativeBuildDir / "CMakeFiles" / (targetNode->displayName() + ".dir");
    } else if (!generator.contains("Makefiles")) {
        Core::MessageManager::writeFlashing(addCMakePrefix(
            Tr::tr("Build File is not supported for generator \"%1\"").arg(generator)));
        return;
    }

    auto cbc = static_cast<CMakeBuildSystem *>(bc->buildSystem());
    const QString sourceFile = targetBase.pathAppended(relativeSource).toString();
    const QString objExtension = [&]() -> QString {
        const auto sourceKind = ProjectFile::classify(relativeSource);
        const QByteArray cmakeLangExtension = ProjectFile::isCxx(sourceKind)
                                                  ? "CMAKE_CXX_OUTPUT_EXTENSION"
                                                  : "CMAKE_C_OUTPUT_EXTENSION";
        const QString extension = cbc->configurationFromCMake().stringValueOf(cmakeLangExtension);
        if (!extension.isEmpty())
            return extension;

        const auto toolchain = ProjectFile::isCxx(sourceKind)
                                   ? ToolChainKitAspect::cxxToolChain(target->kit())
                                   : ToolChainKitAspect::cToolChain(target->kit());
        using namespace ProjectExplorer::Constants;
        static QSet<Id> objIds{
            CLANG_CL_TOOLCHAIN_TYPEID,
            MSVC_TOOLCHAIN_TYPEID,
            MINGW_TOOLCHAIN_TYPEID,
        };
        if (objIds.contains(toolchain->typeId()))
            return ".obj";
        return ".o";
    }();

    cbc->buildCMakeTarget(sourceFile + objExtension);
}

void CMakeManager::buildFileContextMenu()
{
    if (Node *node = ProjectTree::currentNode())
        buildFile(node);
}

} // CMakeProjectManager::Internal
