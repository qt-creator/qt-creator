// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimproject.h"

#include "../nimconstants.h"
#include "../nimtr.h"
#include "nimbleproject.h"
#include "nimcompilerbuildstep.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const char SETTINGS_KEY[] = "Nim.BuildSystem";
const char EXCLUDED_FILES_KEY[] = "ExcludedFiles";

NimProjectScanner::NimProjectScanner(Project *project)
    : m_project(project)
{
    connect(&m_directoryWatcher, &FileSystemWatcher::directoryChanged,
            this, &NimProjectScanner::directoryChanged);
    connect(&m_directoryWatcher, &FileSystemWatcher::fileChanged,
            this, &NimProjectScanner::fileChanged);

    connect(m_project, &Project::settingsLoaded, this, &NimProjectScanner::loadSettings);
    connect(m_project, &Project::aboutToSaveSettings, this, &NimProjectScanner::saveSettings);

    connect(&m_scanner, &TreeScanner::finished, this, [this] {
        // Collect scanned nodes
        std::vector<std::unique_ptr<FileNode>> nodes;
        TreeScanner::Result scanResult = m_scanner.release();
        for (FileNode *node : scanResult.takeAllFiles()) {
            if (!node->path().endsWith(".nim") && !node->path().endsWith(".nimble"))
                node->setEnabled(false); // Disable files that do not end in .nim
            nodes.emplace_back(node);
        }

        // Sync watched dirs
        const QSet<FilePath> fsDirs = Utils::transform<QSet>(nodes,
                                                             [](const std::unique_ptr<FileNode> &fn) { return fn->directory(); });
        const QSet<FilePath> projectDirs = Utils::toSet(m_directoryWatcher.directories());
        m_directoryWatcher.addDirectories(Utils::toList(fsDirs - projectDirs), FileSystemWatcher::WatchAllChanges);
        m_directoryWatcher.removeDirectories(Utils::toList(projectDirs - fsDirs));

        // Sync project files
        const QSet<FilePath> fsFiles = Utils::transform<QSet>(nodes, &FileNode::filePath);
        const QSet<FilePath> projectFiles = Utils::toSet(m_project->files([](const Node *n) { return Project::AllFiles(n); }));

        if (fsFiles != projectFiles) {
            auto projectNode = std::make_unique<ProjectNode>(m_project->projectDirectory());
            projectNode->setDisplayName(m_project->displayName());
            projectNode->addNestedNodes(std::move(nodes));
            m_project->setRootProjectNode(std::move(projectNode));
        }

        emit finished();
    });
}

void NimProjectScanner::loadSettings()
{
    QVariantMap settings = m_project->namedSettings(SETTINGS_KEY).toMap();
    if (settings.contains(EXCLUDED_FILES_KEY))
        setExcludedFiles(settings.value(EXCLUDED_FILES_KEY, excludedFiles()).toStringList());

    emit requestReparse();
}

void NimProjectScanner::saveSettings()
{
    QVariantMap settings;
    settings.insert(EXCLUDED_FILES_KEY, excludedFiles());
    m_project->setNamedSettings(SETTINGS_KEY, settings);
}

void NimProjectScanner::startScan()
{
    m_scanner.setFilter(
        [excludedFiles = excludedFiles()](const Utils::MimeType &, const FilePath &fp) {
            const QString path = fp.toUrlishString();
            return excludedFiles.contains(path) || path.endsWith(".nimproject")
                   || path.contains(".nimproject.user") || path.contains(".nimble.user");
        });

    m_scanner.asyncScanForFiles(m_project->projectDirectory());
}

void NimProjectScanner::watchProjectFilePath()
{
    m_directoryWatcher.addFile(m_project->projectFilePath(), FileSystemWatcher::WatchModifiedDate);
}

void NimProjectScanner::setExcludedFiles(const QStringList &list)
{
    static_cast<NimbleProject *>(m_project)->setExcludedFiles(list);
}

QStringList NimProjectScanner::excludedFiles() const
{
    return static_cast<NimbleProject *>(m_project)->excludedFiles();
}

bool NimProjectScanner::addFiles(const QStringList &filePaths)
{
    setExcludedFiles(Utils::filtered(excludedFiles(), [&](const QString & f) {
        return !filePaths.contains(f);
    }));

    emit requestReparse();

    return true;
}

RemovedFilesFromProject NimProjectScanner::removeFiles(const QStringList &filePaths)
{
    setExcludedFiles(Utils::filteredUnique(excludedFiles() + filePaths));

    emit requestReparse();

    return RemovedFilesFromProject::Ok;
}

bool NimProjectScanner::renameFile(const QString &, const QString &to)
{
    QStringList files = excludedFiles();
    files.removeOne(to);
    setExcludedFiles(files);

    emit requestReparse();

    return true;
}

// NimBuildSystem

class NimBuildSystem final : public BuildSystem
{
public:
    explicit NimBuildSystem(BuildConfiguration *bc);
    ~NimBuildSystem();

    bool supportsAction(Node *, ProjectAction action, const Node *node) const final;
    bool addFiles(Node *node, const FilePaths &filePaths, FilePaths *) final;
    RemovedFilesFromProject removeFiles(Node *node,
                                        const FilePaths &filePaths,
                                        FilePaths *) final;
    bool deleteFiles(Node *, const FilePaths &) final;
    bool renameFiles(
        Node *,
        const Utils::FilePairs &filesToRename,
        Utils::FilePaths *notRenamed) final;
    void triggerParsing() final;

protected:
    ParseGuard m_guard;
    NimProjectScanner m_projectScanner;
};

NimBuildSystem::NimBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc), m_projectScanner(bc->project())
{
    connect(&m_projectScanner, &NimProjectScanner::finished, this, [this] {
        m_guard.markAsSuccess();
        m_guard = {}; // Trigger destructor of previous object, emitting parsingFinished()

        emitBuildSystemUpdated();
    });

    connect(&m_projectScanner, &NimProjectScanner::requestReparse,
            this, &NimBuildSystem::requestDelayedParse);

    connect(&m_projectScanner, &NimProjectScanner::directoryChanged, this, [this] {
        if (!isWaitingForParse())
            requestDelayedParse();
    });

    requestDelayedParse();
}

NimBuildSystem::~NimBuildSystem()
{
    // Trigger any pending parsingFinished signals before destroying any other build system part:
    m_guard = {};
}

void NimBuildSystem::triggerParsing()
{
    m_guard = guardParsingRun();
    m_projectScanner.startScan();
}

FilePath nimPathFromKit(Kit *kit)
{
    auto tc = ToolchainKitAspect::toolchain(kit, Constants::C_NIMLANGUAGE_ID);
    QTC_ASSERT(tc, return {});
    const FilePath command = tc->compilerCommand();
    return command.isEmpty() ? FilePath() : command.absolutePath();
}

FilePath nimblePathFromKit(Kit *kit)
{
    // There's no extra setting for "nimble", derive it from the "nim" path.
    const FilePath nimbleFromPath = FilePath("nimble").searchInPath();
    const FilePath nimPath = nimPathFromKit(kit);
    const FilePath nimbleFromKit = nimPath.pathAppended("nimble").withExecutableSuffix();
    return nimbleFromKit.exists() ? nimbleFromKit.canonicalPath() : nimbleFromPath;
}

bool NimBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (node->asFileNode()) {
        return action == ProjectAction::Rename
               || action == ProjectAction::RemoveFile;
    }
    if (node->isFolderNodeType() || node->isProjectNodeType()) {
        return action == ProjectAction::AddNewFile
               || action == ProjectAction::RemoveFile
               || action == ProjectAction::AddExistingFile;
    }
    return BuildSystem::supportsAction(context, action, node);
}

bool NimBuildSystem::addFiles(Node *, const FilePaths &filePaths, FilePaths *)
{
    return m_projectScanner.addFiles(Utils::transform(filePaths, &FilePath::toUrlishString));
}

RemovedFilesFromProject NimBuildSystem::removeFiles(Node *,
                                                    const FilePaths &filePaths,
                                                    FilePaths *)
{
    return m_projectScanner.removeFiles(Utils::transform(filePaths, &FilePath::toUrlishString));
}

bool NimBuildSystem::deleteFiles(Node *, const FilePaths &)
{
    return true;
}

bool NimBuildSystem::renameFiles(Node *, const FilePairs &filesToRename, FilePaths *notRenamed)
{
    bool success = true;
    for (const auto &[oldFilePath, newFilePath] : filesToRename) {
        if (!m_projectScanner.renameFile(oldFilePath.toUrlishString(), newFilePath.toUrlishString())) {
            success = false;
            if (notRenamed)
                *notRenamed << oldFilePath;
        }
    }
    return success;
}

static FilePath defaultBuildDirectory(const Kit *k,
                                      const FilePath &projectFilePath,
                                      const QString &bc,
                                      BuildConfiguration::BuildType buildType)
{
    return BuildConfiguration::buildDirectoryFromTemplate(
        projectFilePath.parentDir(), projectFilePath, projectFilePath.baseName(),
        k, bc, buildType, "nim");
}

NimBuildConfiguration::NimBuildConfiguration(Target *target, Utils::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(Tr::tr("General"));
    setConfigWidgetHasFrame(true);
    setBuildDirectorySettingsKey("Nim.NimBuildConfiguration.BuildDirectory");

    appendInitialBuildStep(Constants::C_NIMCOMPILERBUILDSTEP_ID);
    appendInitialCleanStep(Constants::C_NIMCOMPILERCLEANSTEP_ID);

    setInitializer([this](const BuildInfo &info) {
        // Create the build configuration and initialize it from build info
        setBuildDirectory(defaultBuildDirectory(kit(),
                                                project()->projectFilePath(),
                                                displayName(),
                                                buildType()));

        auto nimCompilerBuildStep = buildSteps()->firstOfType<NimCompilerBuildStep>();
        QTC_ASSERT(nimCompilerBuildStep, return);
        nimCompilerBuildStep->setBuildType(info.buildType);
    });
}

FilePath NimBuildConfiguration::cacheDirectory() const
{
    return buildDirectory().pathAppended("nimcache");
}

FilePath NimBuildConfiguration::outFilePath() const
{
    auto nimCompilerBuildStep = buildSteps()->firstOfType<NimCompilerBuildStep>();
    QTC_ASSERT(nimCompilerBuildStep, return {});
    return nimCompilerBuildStep->outFilePath();
}

// NimBuildConfigurationFactory

class NimBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    NimBuildConfigurationFactory()
    {
        registerBuildConfiguration<NimBuildConfiguration>(Constants::C_NIMBUILDCONFIGURATION_ID);
        setSupportedProjectType(Constants::C_NIMPROJECT_ID);
        setSupportedProjectMimeTypeName(Constants::C_NIM_PROJECT_MIMETYPE);

        setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
            const auto oneBuild = [&](BuildConfiguration::BuildType buildType, const QString &typeName) {
                BuildInfo info;
                info.buildType = buildType;
                info.typeName = typeName;
                if (forSetup) {
                    info.displayName = info.typeName;
                    info.buildDirectory = defaultBuildDirectory(k, projectPath, info.typeName, buildType);
                }
                info.enabledByDefault = buildType == BuildConfiguration::Debug;
                return info;
            };
            return QList<BuildInfo>{
                oneBuild(BuildConfiguration::Debug, Tr::tr("Debug")),
                oneBuild(BuildConfiguration::Release, Tr::tr("Release"))
            };
        });
    }
};


class NimProject final : public Project
{
public:
    explicit NimProject(const FilePath &filePath);

    // Keep for compatibility with Qt Creator 4.10
    void toMap(Store &map) const final;

protected:
    // Keep for compatibility with Qt Creator 4.10
    RestoreResult fromMap(const Store &map, QString *errorMessage) final;

    QStringList m_excludedFiles;
};

NimProject::NimProject(const FilePath &filePath) : Project(Constants::C_NIM_MIMETYPE, filePath)
{
    setId(Constants::C_NIMPROJECT_ID);
    setDisplayName(filePath.completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setBuildSystemCreator<NimBuildSystem>("nim");
}

void NimProject::toMap(Store &map) const
{
    Project::toMap(map);
    map[Constants::C_NIMPROJECT_EXCLUDEDFILES] = m_excludedFiles;
}

Project::RestoreResult NimProject::fromMap(const Store &map, QString *errorMessage)
{
    auto result = Project::fromMap(map, errorMessage);
    m_excludedFiles = map.value(Constants::C_NIMPROJECT_EXCLUDEDFILES).toStringList();
    return result;
}

// Setup

void setupNimProject()
{
    static const NimBuildConfigurationFactory buildConfigFactory;
    const auto issuesGenerator = [](const Kit *k) {
        Tasks result;
        auto tc = ToolchainKitAspect::toolchain(k, Constants::C_NIMLANGUAGE_ID);
        if (!tc) {
            result.append(
                Project::createTask(Task::TaskType::Error, Tr::tr("No Nim compiler set.")));
            return result;
        }
        if (!tc->compilerCommand().exists())
            result.append(
                Project::createTask(Task::TaskType::Error, Tr::tr("Nim compiler does not exist.")));
        return result;
    };
    ProjectManager::registerProjectType<NimProject>(
        Constants::C_NIM_PROJECT_MIMETYPE, issuesGenerator);
}

} // Nim
