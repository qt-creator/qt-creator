// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectmanager.h"

#include "cmakebuildsystem.h"
#include "cmakebuildconfiguration.h"
#include "cmakekitaspect.h"
#include "cmakeprocess.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeprojectnodes.h"
#include "cmakespecificsettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>

#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>

#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <utils/action.h>
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

private:
    void updateCMakeActions(Node *node);
    void updateCMakeBuildTarget(Node *node);
    void clearCMakeCache(BuildSystem *buildSystem);
    void runCMake(BuildSystem *buildSystem);
    void runCMakeWithProfiling(BuildSystem *buildSystem);
    void rescanProject(BuildSystem *buildSystem);
    void buildFileContextMenu();
    void buildFile(Node *node = nullptr);
    void updateBuildFileAction();
    void enableBuildFileMenus(Node *node);
    void reloadCMakePresets();
    void enableBuildSubprojectContextMenu(Node *node);
    void enableBuildSubprojectMenu();
    void runSubprojectOperation(const QString &clean, const QString &build);
    const CMakeListsNode* currentListsNodeForEditor();

    QAction *m_runCMakeAction;
    QAction *m_clearCMakeCacheAction;
    QAction *m_runCMakeActionContextMenu;
    QAction *m_clearCMakeCacheActionContextMenu;
    QAction *m_rescanProjectAction;
    QAction *m_buildFileContextMenu;
    QAction *m_reloadCMakePresetsAction;
    Utils::Action *m_buildFileAction;
    QAction *m_cmakeProfilerAction;
    QAction *m_cmakeDebuggerAction;
    QAction *m_cmakeDebuggerSeparator;
    bool m_canDebugCMake = false;
    Action *m_buildSubprojectContextAction = nullptr;
    QAction *m_cleanSubprojectContextAction = nullptr;
    QAction *m_rebuildSubprojectContextAction = nullptr;
    Action *m_buildSubprojectAction = nullptr;
    QAction *m_cleanSubprojectAction = nullptr;
    QAction *m_rebuildSubprojectAction = nullptr;
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
        QUrl(urlPrefix + address.mid(address.lastIndexOf(doc) + doc.length())));
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
        .addOnTriggered(this, [this] { runCMake(ProjectManager::startupBuildSystem()); });

    ActionBuilder(this, Constants::CLEAR_CMAKE_CACHE)
        .setText(Tr::tr("Clear CMake Configuration"))
        .bindContextAction(&m_clearCMakeCacheAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] { clearCMakeCache(ProjectManager::startupBuildSystem()); });

    ActionBuilder(this, Constants::RUN_CMAKE_CONTEXT_MENU)
        .setText(Tr::tr("Run CMake"))
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .setContext(projectContext)
        .bindContextAction(&m_runCMakeActionContextMenu)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_BUILD)
        .addOnTriggered(this, [this] { runCMake(ProjectTree::currentBuildSystem()); });

    ActionBuilder(this, Constants::CLEAR_CMAKE_CACHE_CONTEXT_MENU)
        .setText(Tr::tr("Clear CMake Configuration"))
        .setContext(projectContext)
        .bindContextAction(&m_clearCMakeCacheActionContextMenu)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_REBUILD)
        .addOnTriggered(this, [this] { clearCMakeCache(ProjectManager::startupBuildSystem()); });

    ActionBuilder(this, Constants::BUILD_FILE_CONTEXT_MENU)
        .setText(Tr::tr("Build"))
        .bindContextAction(&m_buildFileContextMenu)
        .setContext(projectContext)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_FILECONTEXT, PEC::G_FILE_OTHER)
        .addOnTriggered(this, [this] { buildFileContextMenu(); });

    ActionBuilder(this, Constants::RESCAN_PROJECT)
        .setText(Tr::tr("Rescan Project"))
        .bindContextAction(&m_rescanProjectAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] { rescanProject(ProjectTree::currentBuildSystem()); });

    ActionBuilder(this, Constants::RELOAD_CMAKE_PRESETS)
        .setText(Tr::tr("Reload CMake Presets"))
        .setIcon(Utils::Icons::RELOAD.icon())
        .bindContextAction(&m_reloadCMakePresetsAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] { reloadCMakePresets(); });

    ActionBuilder(this, Constants::BUILD_FILE)
        .setParameterText(Tr::tr("Build File \"%1\""), Tr::tr("Build File"),
                          ActionBuilder::AlwaysEnabled)
        .bindContextAction(&m_buildFileAction)
        .setCommandAttribute(Command::CA_Hide)
        .setCommandAttribute(Command::CA_UpdateText)
        .setCommandDescription(m_buildFileAction->text())
        .setDefaultKeySequence(Tr::tr("Ctrl+Alt+B"))
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] { buildFile(); });

    // Subproject
    ActionBuilder(this, Constants::BUILD_SUBPROJECT)
        .setParameterText(
            Tr::tr("Build &Subproject \"%1\""),
            Tr::tr("Build &Subproject"),
            ActionBuilder::AlwaysEnabled)
        .setContext(projectContext)
        .bindContextAction(&m_buildSubprojectAction)
        .setCommandAttribute(Command::CA_Hide)
        .setCommandAttribute(Command::CA_UpdateText)
        .setCommandDescription(m_buildSubprojectAction->text())
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_SUBPROJECT)
        .addOnTriggered(this, [&] { runSubprojectOperation(QString(), "all"); });

    ActionBuilder(this, Constants::REBUILD_SUBPROJECT)
        .setText(Tr::tr("Rebuild"))
        .setContext(projectContext)
        .bindContextAction(&m_rebuildSubprojectAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_SUBPROJECT)
        .addOnTriggered(this, [&] { runSubprojectOperation("clean", "all"); });

    ActionBuilder(this, Constants::CLEAN_SUBPROJECT)
        .setText(Tr::tr("Clean"))
        .setContext(projectContext)
        .bindContextAction(&m_cleanSubprojectAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_SUBPROJECT)
        .addOnTriggered(this, [&] { runSubprojectOperation("clean", QString()); });

    // Subproject context menus
    ActionBuilder(this, Constants::BUILD_SUBPROJECT_CONTEXT_MENU)
        .setParameterText(
            Tr::tr("Build &Subproject \"%1\""),
            Tr::tr("Build &Subproject"),
            ActionBuilder::AlwaysEnabled)
        .setContext(projectContext)
        .bindContextAction(&m_buildSubprojectContextAction)
        .setCommandAttribute(Command::CA_Hide)
        .setCommandAttribute(Command::CA_UpdateText)
        .setCommandDescription(m_buildSubprojectContextAction->text())
        .addToContainer(PEC::M_SUBPROJECTCONTEXT, PEC::G_PROJECT_BUILD)
        .addOnTriggered(this, [&] { runSubprojectOperation(QString(), "all"); });

    ActionBuilder(this, Constants::REBUILD_SUBPROJECT_CONTEXT_MENU)
        .setText(Tr::tr("Rebuild"))
        .setContext(projectContext)
        .bindContextAction(&m_rebuildSubprojectContextAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_SUBPROJECTCONTEXT, PEC::G_PROJECT_BUILD)
        .addOnTriggered(this, [&] { runSubprojectOperation("clean", "all"); });

    ActionBuilder(this, Constants::CLEAN_SUBPROJECT_CONTEXT_MENU)
        .setText(Tr::tr("Clean"))
        .setContext(projectContext)
        .bindContextAction(&m_cleanSubprojectContextAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_SUBPROJECTCONTEXT, PEC::G_PROJECT_BUILD)
        .addOnTriggered(this, [&] { runSubprojectOperation("clean", QString()); });

    // CMake Profiler
    ActionBuilder(this, Constants::RUN_CMAKE_PROFILER)
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .setText(Tr::tr("CMake Profiler"))
        .bindContextAction(&m_cmakeProfilerAction)
        .addToContainer(Debugger::Constants::M_DEBUG_ANALYZER,
                        Debugger::Constants::G_ANALYZER_TOOLS,
                        false)
        .addOnTriggered(this, [this] {
            runCMakeWithProfiling(ProjectManager::startupBuildSystem());
        });

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
        auto cmakeBuildSystem = qobject_cast<CMakeBuildSystem *>(
            ProjectManager::startupBuildSystem());
        if (cmakeBuildSystem) {
            const BuildDirParameters parameters(cmakeBuildSystem);
            const auto tool = parameters.cmakeTool();
            CMakeTool::Version version = tool ? tool->version() : CMakeTool::Version();
            m_canDebugCMake = (version.major == 3 && version.minor >= 27) || version.major > 3;
        }
        updateCMakeActions(ProjectTree::currentNode());
    });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this, [this] {
        updateCMakeActions(ProjectTree::currentNode());
    });
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged, this, [this]() {
        updateBuildFileAction();
        enableBuildSubprojectMenu();
    });
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged, this, [this](Node *node) {
        updateCMakeActions(node);
        updateCMakeBuildTarget(node);
    });

    updateCMakeActions(ProjectTree::currentNode());
}

void CMakeManager::updateCMakeActions(Node *node)
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

    enableBuildFileMenus(node);
    enableBuildSubprojectContextMenu(node);
}

void CMakeManager::updateCMakeBuildTarget(Node *node)
{
    if (!node)
        return;

    auto bs = qobject_cast<CMakeBuildSystem *>(ProjectTree::currentBuildSystem());
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
        QObject::connect(cmakeBuildSystem->target(), &Target::buildSystemUpdated, this, [] {
            Core::Command *ctfVisualiserLoadTrace = Core::ActionManager::command(
                "Analyzer.Menu.StartAnalyzer.CtfVisualizer.LoadTrace");

            if (ctfVisualiserLoadTrace) {
                auto *action = ctfVisualiserLoadTrace->actionForContext(Core::Constants::C_GLOBAL);
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
    CMakeProject *project = qobject_cast<CMakeProject *>(ProjectTree::currentProject());
    if (!project)
        return;

    QMessageBox::StandardButton clickedButton = CheckableMessageBox::question(
        Core::ICore::dialogParent(),
        Tr::tr("Reload CMake Presets"),
        Tr::tr("Re-generates the kits that were created for CMake presets. All manual "
               "modifications to the CMake project settings will be lost."),
        settings(project).askBeforePresetsReload.askAgainCheckableDecider(),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Yes,
        QMessageBox::Yes,
        {
            {QMessageBox::Yes, Tr::tr("Reload")},
        });

    settings(project).writeSettings();

    if (clickedButton == QMessageBox::Cancel)
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

    emit project->projectImporter()->cmakePresetsUpdated();

    Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    Core::ModeManager::setFocusToCurrentMode();
}

void CMakeManager::enableBuildSubprojectContextMenu(Node *node)
{
    const Project *project = ProjectTree::projectForNode(node);
    auto subProjectNode = dynamic_cast<const CMakeListsNode *>(node);

    const QString subProjectDisplayName = subProjectNode ? subProjectNode->displayName()
                                                         : QString();
    const bool subProjectVisible = subProjectNode && subProjectNode->hasSubprojectBuildSupport()
                                   && !BuildManager::isBuilding(project);

    m_buildSubprojectContextAction->setParameter(subProjectDisplayName);
    m_buildSubprojectContextAction->setEnabled(subProjectVisible);
    m_buildSubprojectContextAction->setVisible(subProjectVisible);

    m_cleanSubprojectContextAction->setEnabled(subProjectVisible);
    m_cleanSubprojectContextAction->setVisible(subProjectVisible);

    m_rebuildSubprojectContextAction->setEnabled(subProjectVisible);
    m_rebuildSubprojectContextAction->setVisible(subProjectVisible);
}

void CMakeManager::enableBuildSubprojectMenu()
{
    auto subProjectNode = currentListsNodeForEditor();
    const Project *project = ProjectTree::projectForNode(subProjectNode);

    const QString subProjectDisplayName = subProjectNode ? subProjectNode->displayName()
                                                         : QString();
    const bool subProjectVisible = subProjectNode && subProjectNode->hasSubprojectBuildSupport()
                                   && !BuildManager::isBuilding(project);

    m_buildSubprojectAction->setParameter(subProjectDisplayName);
    m_buildSubprojectAction->setEnabled(subProjectVisible);
    m_buildSubprojectAction->setVisible(subProjectVisible);

    m_cleanSubprojectAction->setEnabled(subProjectVisible);
    m_cleanSubprojectAction->setVisible(subProjectVisible);

    m_rebuildSubprojectAction->setEnabled(subProjectVisible);
    m_rebuildSubprojectAction->setVisible(subProjectVisible);
}

void CMakeManager::runSubprojectOperation(const QString &clean, const QString &build)
{
    if (auto bs = qobject_cast<CMakeBuildSystem *>(ProjectTree::currentBuildSystem())) {
        auto subProject = dynamic_cast<const CMakeListsNode *>(ProjectTree::currentNode());

        // We want to allow the build action from a source file when triggered from a keyboard seqnuence
        if (!subProject)
            subProject = currentListsNodeForEditor();

        if (!subProject)
            return;

        auto subProjectDir = subProject->filePath();
        auto projectFileDir = bs->project()->projectFilePath().parentDir();

        auto relativePath = subProjectDir.relativeChildPath(projectFileDir);
        if (clean.isEmpty()) {
            bs->buildCMakeTarget(relativePath.path() + "/" + build);
        } else if (build.isEmpty()) {
            bs->buildCMakeTarget(relativePath.path() + "/" + clean);
        } else {
            bs->reBuildCMakeTarget(
                relativePath.path() + "/" + clean, relativePath.path() + "/" + build);
        }
    }
}

const CMakeListsNode *CMakeManager::currentListsNodeForEditor()
{
    Core::IDocument *currentDocument = Core::EditorManager::currentDocument();
    if (!currentDocument)
        return nullptr;

    const FilePath file = currentDocument->filePath();
    Node *n = ProjectTree::nodeForFile(file);
    FileNode *node = n ? n->asFileNode() : nullptr;
    if (!node)
        return nullptr;
    auto targetNode = dynamic_cast<const CMakeTargetNode *>(node->parentProjectNode());
    if (!targetNode)
        return nullptr;

    auto bs = qobject_cast<CMakeBuildSystem *>(ProjectTree::currentBuildSystem());
    if (!bs)
        return nullptr;

    auto cmakeBuildTarget
        = Utils::findOrDefault(bs->buildTargets(), [&targetNode](const CMakeBuildTarget &cbt) {
              return targetNode->buildKey() == cbt.title;
          });

    if (cmakeBuildTarget.backtrace.isEmpty())
        return nullptr;
    const FilePath targetDefinitionDir = cmakeBuildTarget.backtrace.last().path.parentDir();

    auto projectNode = bs->project()->rootProjectNode()->findProjectNode(
        [&targetDefinitionDir](const ProjectNode *node) {
            if (auto cmakeListsNode = dynamic_cast<const CMakeListsNode *>(node))
                return cmakeListsNode->path() == targetDefinitionDir;
            return false;
        });
    return dynamic_cast<CMakeListsNode *>(projectNode);
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
    const FilePath relativeSource = filePath.relativeChildPath(targetNode->filePath());
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
    const QString sourceFile = targetBase.resolvePath(relativeSource).path();
    const QString objExtension = [&]() -> QString {
        const auto sourceKind = ProjectFile::classify(relativeSource);
        const QByteArray cmakeLangExtension = ProjectFile::isCxx(sourceKind)
                                                  ? "CMAKE_CXX_OUTPUT_EXTENSION"
                                                  : "CMAKE_C_OUTPUT_EXTENSION";
        const QString extension = cbc->configurationFromCMake().stringValueOf(cmakeLangExtension);
        if (!extension.isEmpty())
            return extension;

        const auto toolchain = ProjectFile::isCxx(sourceKind)
                                   ? ToolchainKitAspect::cxxToolchain(target->kit())
                                   : ToolchainKitAspect::cToolchain(target->kit());
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

void setupCMakeManager()
{
    static CMakeManager theCMakeManager;
}

void setupOnlineHelpManager()
{
    Core::HelpManager::addOnlineHelpHandler({CMakeManager::isCMakeUrl, CMakeManager::openCMakeUrl});
}

} // CMakeProjectManager::Internal
