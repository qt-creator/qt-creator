// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectmanager.h"

#include "cmakebuildsystem.h"
#include "cmakekitaspect.h"
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
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
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

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

CMakeManager::CMakeManager()
    : m_runCMakeAction(new QAction(QIcon(), Tr::tr("Run CMake"), this))
    , m_clearCMakeCacheAction(new QAction(QIcon(), Tr::tr("Clear CMake Configuration"), this))
    , m_runCMakeActionContextMenu(new QAction(QIcon(), Tr::tr("Run CMake"), this))
    , m_rescanProjectAction(new QAction(QIcon(), Tr::tr("Rescan Project"), this))
    , m_reloadCMakePresetsAction(
          new QAction(Utils::Icons::RELOAD.icon(), Tr::tr("Reload CMake Presets"), this))
    , m_cmakeProfilerAction(new QAction(QIcon(), Tr::tr("CMake Profiler"), this))
{
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    Core::ActionContainer *mfile =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);
    Core::ActionContainer *manalyzer =
        Core::ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);

    const Core::Context projectContext(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    const Core::Context globalContext(Core::Constants::C_GLOBAL);

    Core::Command *command = Core::ActionManager::registerAction(m_runCMakeAction,
                                                                 Constants::RUN_CMAKE,
                                                                 globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_runCMakeAction, &QAction::triggered, this, [this] {
        runCMake(ProjectManager::startupBuildSystem());
    });

    command = Core::ActionManager::registerAction(m_clearCMakeCacheAction,
                                                  Constants::CLEAR_CMAKE_CACHE,
                                                  globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_clearCMakeCacheAction, &QAction::triggered, this, [this] {
        clearCMakeCache(ProjectManager::startupBuildSystem());
    });

    command = Core::ActionManager::registerAction(m_runCMakeActionContextMenu,
                                                  Constants::RUN_CMAKE_CONTEXT_MENU,
                                                  projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runCMakeActionContextMenu, &QAction::triggered, this, [this] {
        runCMake(ProjectTree::currentBuildSystem());
    });

    m_buildFileContextMenu = new QAction(Tr::tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildFileContextMenu,
                                                  Constants::BUILD_FILE_CONTEXT_MENU,
                                                  projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileContextMenu, &QAction::triggered,
            this, &CMakeManager::buildFileContextMenu);

    command = Core::ActionManager::registerAction(m_rescanProjectAction,
                                                  Constants::RESCAN_PROJECT,
                                                  globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_rescanProjectAction, &QAction::triggered, this, [this] {
        rescanProject(ProjectTree::currentBuildSystem());
    });

    command = Core::ActionManager::registerAction(m_reloadCMakePresetsAction,
                                                  Constants::RELOAD_CMAKE_PRESETS,
                                                  globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_reloadCMakePresetsAction, &QAction::triggered, this, [this] {
        reloadCMakePresets();
    });

    m_buildFileAction = new Utils::ParameterAction(Tr::tr("Build File"),
                                                   Tr::tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled,
                                                   this);
    command = Core::ActionManager::registerAction(m_buildFileAction, Constants::BUILD_FILE);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFileAction, &QAction::triggered, this, [this] { buildFile(); });

    // CMake Profiler
    command = Core::ActionManager::registerAction(m_cmakeProfilerAction,
                                                  Constants::RUN_CMAKE_PROFILER,
                                                  globalContext);
    command->setDescription(m_cmakeProfilerAction->text());
    manalyzer->addAction(command, Debugger::Constants::G_ANALYZER_TOOLS);
    connect(m_cmakeProfilerAction, &QAction::triggered, this, [this] {
        runCMakeWithProfiling(ProjectManager::startupBuildSystem());
    });

    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged, this, [this] {
        updateCmakeActions(ProjectTree::currentNode());
    });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this, [this] {
        updateCmakeActions(ProjectTree::currentNode());
    });
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
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
    const QString objExtension = Utils::HostOsInfo::isWindowsHost() ? QString(".obj") : QString(".o");
    Utils::FilePath targetBase;
    BuildConfiguration *bc = target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    if (generator == "Ninja") {
        const Utils::FilePath relativeBuildDir = targetNode->buildDirectory().relativeChildPath(
                    bc->buildDirectory());
        targetBase = relativeBuildDir / "CMakeFiles" / (targetNode->displayName() + ".dir");
    } else if (!generator.contains("Makefiles")) {
        Core::MessageManager::writeFlashing(
            Tr::tr("Build File is not supported for generator \"%1\"").arg(generator));
        return;
    }

    static_cast<CMakeBuildSystem *>(bc->buildSystem())
            ->buildCMakeTarget(targetBase.pathAppended(relativeSource).toString() + objExtension);
}

void CMakeManager::buildFileContextMenu()
{
    if (Node *node = ProjectTree::currentNode())
        buildFile(node);
}

} // CMakeProjectManager::Internal
