// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "workspaceproject.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildsteplist.h"
#include "buildsystem.h"
#include "processstep.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projecttree.h"
#include "runconfiguration.h"
#include "runconfigurationaspects.h"
#include "runcontrol.h"
#include "target.h"
#include "treescanner.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Utils;
using namespace Core;

namespace ProjectExplorer {

Q_LOGGING_CATEGORY(wsbs, "qtc.projectexplorer.workspace.buildsystem", QtWarningMsg);
Q_LOGGING_CATEGORY(wsp, "qtc.projectexplorer.workspace.project", QtWarningMsg);

const QLatin1StringView FOLDER_MIMETYPE{"inode/directory"};
const QLatin1StringView WORKSPACE_MIMETYPE{"text/x-workspace-project"};
const char WORKSPACE_PROJECT_ID[] = "ProjectExplorer.WorkspaceProject";
const char WORKSPACE_PROJECT_RUNCONFIG_ID[] = "WorkspaceProject.RunConfiguration:";

const QLatin1StringView PROJECT_NAME_KEY{"project.name"};
const QLatin1StringView FILES_EXCLUDE_KEY{"files.exclude"};
const char EXCLUDE_ACTION_ID[] = "ProjectExplorer.ExcludeFromWorkspace";
const char RESCAN_ACTION_ID[] = "ProjectExplorer.RescanWorkspace";

const expected_str<QJsonObject> projectDefinition(const FilePath &path)
{
    if (auto fileContents = path.fileContents())
        return QJsonDocument::fromJson(*fileContents).object();
    return {};
}

static QFlags<QDir::Filter> workspaceDirFilter = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden;

class WorkspaceBuildSystem final : public BuildSystem
{
public:
    WorkspaceBuildSystem(Target *t);

    void reparse(bool force);
    void triggerParsing() final;

    bool addFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded) final;
    RemovedFilesFromProject removeFiles(Node *context, const FilePaths &filePaths, FilePaths *notRemoved) final;
    bool deleteFiles(Node *context, const FilePaths &filePaths) final;
    bool renameFiles(Node *context, const FilePairs &filesToRename, FilePaths *notRenamed) final;
    bool supportsAction(Node *context, ProjectAction action, const Node *node) const final;

    void handleDirectoryChanged(const FilePath &directory);

    void scan(const FilePath &path);
    void scanNext();

    QString name() const final { return QLatin1String("Workspace"); }

private:
    bool isFiltered(const FilePath &path, QList<IVersionControl *> versionControls) const;

    QList<QRegularExpression> m_filters;
    std::unique_ptr<FileSystemWatcher> m_watcher;
    ParseGuard m_parseGuard;
    FilePaths m_scanQueue;
    TreeScanner m_scanner;
};

static FolderNode *findAvailableParent(ProjectNode *root, const FilePath &path)
{
    FolderNode *result = nullptr;
    FolderNode *child = root;
    do {
        result = child;
        child = result->findChildFolderNode(
            [&path](FolderNode *fn) { return path.isChildOf(fn->path()) || path == fn->path(); });
    } while (child);
    return result;
}

WorkspaceBuildSystem::WorkspaceBuildSystem(Target *t)
    :BuildSystem(t)
{
    connect(&m_scanner, &TreeScanner::finished, this, [this] {
        QTC_ASSERT(!m_scanQueue.isEmpty(), return);
        const FilePath scannedDir = m_scanQueue.takeFirst();

        TreeScanner::Result result = m_scanner.release();
        auto addNodes = [this, &result](FolderNode *parent) {
            QElapsedTimer timer;
            timer.start();
            const QList<IVersionControl *> versionControls = VcsManager::versionControls();
            for (auto node : result.takeFirstLevelNodes())
                parent->addNode(std::unique_ptr<Node>(node));
            qCDebug(wsbs) << "Added nodes in" << timer.elapsed() << "ms";
            FilePaths toWatch;
            auto collectWatchFolders = [this, &toWatch, &versionControls](FolderNode *fn) {
                if (!isFiltered(fn->path(), versionControls))
                    toWatch << fn->path();
            };
            collectWatchFolders(parent);
            parent->forEachNode({}, collectWatchFolders);
            qCDebug(wsbs) << "Added and collected nodes in" << timer.elapsed() << "ms" << toWatch.size() << "dirs";
            m_watcher->addDirectories(toWatch, FileSystemWatcher::WatchAllChanges);
            qCDebug(wsbs) << "Added and and collected and watched nodes in" << timer.elapsed() << "ms";
        };

        if (scannedDir == projectDirectory()) {
            qCDebug(wsbs) << "Finished scanning new root" << scannedDir;
            auto root = std::make_unique<ProjectNode>(scannedDir);
            root->setDisplayName(target()->project()->displayName());
            m_watcher.reset(new FileSystemWatcher);
            connect(
                m_watcher.get(),
                &FileSystemWatcher::directoryChanged,
                this,
                [this](const QString &path) {
                    handleDirectoryChanged(FilePath::fromPathPart(path));
                });

            addNodes(root.get());
            setRootProjectNode(std::move(root));
        } else {
            qCDebug(wsbs) << "Finished scanning subdir" << scannedDir;
            FolderNode *parent = findAvailableParent(project()->rootProjectNode(), scannedDir);
            const FilePath relativePath = scannedDir.relativeChildPath(parent->filePath());
            const QString firstRelativeFolder = relativePath.path().left(relativePath.path().indexOf('/'));
            const FilePath nodePath = parent->filePath() / firstRelativeFolder;
            auto newNode = std::make_unique<FolderNode>(nodePath);
            newNode->setDisplayName(firstRelativeFolder);
            addNodes(newNode.get());
            parent->replaceSubtree(nullptr, std::move(newNode));
        }

        scanNext();
    });
    m_scanner.setDirFilter(workspaceDirFilter);
    m_scanner.setFilter([&](const MimeType &, const FilePath &filePath) {
        return Utils::anyOf(m_filters, [filePath](const QRegularExpression &filter) {
            return filter.match(filePath.path()).hasMatch();
        });
    });

    connect(target()->project(),
            &Project::projectFileIsDirty,
            this,
            &BuildSystem::requestDelayedParse);

    requestDelayedParse();
}

void WorkspaceBuildSystem::reparse(bool force)
{
    const QList<QRegularExpression> oldFilters = m_filters;
    m_filters.clear();
    FilePath projectPath = project()->projectDirectory();

    const QJsonObject json = projectDefinition(project()->projectFilePath()).value_or(QJsonObject());
    const QJsonValue projectNameValue = json.value(PROJECT_NAME_KEY);
    if (projectNameValue.isString())
        project()->setDisplayName(projectNameValue.toString());
    const QJsonArray excludesJson = json.value(FILES_EXCLUDE_KEY).toArray();
    for (const QJsonValue &excludeJson : excludesJson) {
        if (excludeJson.isString()) {
            FilePath absolute = projectPath.pathAppended(excludeJson.toString());
            m_filters << QRegularExpression(
                Utils::wildcardToRegularExpression(absolute.path()),
                QRegularExpression::CaseInsensitiveOption);
        }
    }

    QList<BuildTargetInfo> targetInfos;

    const QJsonArray targets = json.value("targets").toArray();
    int i = 0;
    for (const QJsonValue &target : targets) {
        i++;
        QTC_ASSERT(target.isObject(), continue);
        const QJsonObject targetObject = target.toObject();

        QJsonArray args = targetObject["arguments"].toArray();
        QStringList arguments = Utils::transform<QStringList>(args, [](const QJsonValue &arg) {
            return arg.toString();
        });
        FilePath workingDirectory = FilePath::fromUserInput(
            targetObject["workingDirectory"].toString());

        const QString name = targetObject["name"].toString();
        const FilePath executable = FilePath::fromUserInput(
            targetObject["executable"].toString());

        if (name.isEmpty() || executable.isEmpty())
            continue;

        BuildTargetInfo bti;
        bti.buildKey = name + QString::number(i);
        bti.displayName = name;
        bti.displayNameUniquifier = QString(" (%1)").arg(i);
        bti.targetFilePath = executable;
        bti.projectFilePath = projectPath;
        bti.workingDirectory = workingDirectory;
        bti.additionalData = QVariantMap{{"arguments", arguments}};

        targetInfos << bti;
    }

    setApplicationTargets(targetInfos);

    if (force || oldFilters != m_filters)
        scan(target()->project()->projectDirectory());
    else
        emitBuildSystemUpdated();
}

void WorkspaceBuildSystem::triggerParsing()
{
    reparse(false);
}

bool WorkspaceBuildSystem::addFiles(Node *, const FilePaths &, FilePaths *)
{
    // nothing to do here since the changes will be picked up by the file system watcher
    return true;
}

RemovedFilesFromProject WorkspaceBuildSystem::removeFiles(Node *, const FilePaths &, FilePaths *)
{
    // nothing to do here since the changes will be picked up by the file system watcher
    return RemovedFilesFromProject::Ok;
}

bool WorkspaceBuildSystem::deleteFiles(Node *, const FilePaths &)
{
    // nothing to do here since the changes will be picked up by the file system watcher
    return true;
}

bool WorkspaceBuildSystem::renameFiles(Node *, const FilePairs &, FilePaths *)
{
    // nothing to do here since the changes will be picked up by the file system watcher
    return true;
}

bool WorkspaceBuildSystem::supportsAction(Node *, ProjectAction action, const Node *) const
{
    return action == ProjectAction::Rename || action == ProjectAction::AddNewFile
           || action == ProjectAction::EraseFile;
}

void WorkspaceBuildSystem::handleDirectoryChanged(const FilePath &directory)
{
    ProjectNode *root = project()->rootProjectNode();
    QTC_ASSERT(root, return);
    auto node = root->findNode(
        [&directory](Node *node) {
            if (node->asFolderNode()) {
                qCDebug(wsbs) << "comparing" << node->filePath() << directory;
                return node->filePath() == directory;
            }
            return false;
        });
    if (!directory.exists()) {
        m_watcher->removeDirectory(directory);
        if (node)
            node->parentFolderNode()->replaceSubtree(node, nullptr);
    } else if (node) {
        FolderNode *fn = node->asFolderNode();
        QTC_ASSERT(fn, return);
        const FilePaths entries = directory.dirEntries(workspaceDirFilter);
        const QList<IVersionControl *> &versionControls = VcsManager::versionControls();
        for (auto entry : entries) {
            if (isFiltered(entry, versionControls))
                continue;
            if (entry.isDir()) {
                if (!fn->folderNode(entry))
                    scan(entry);
            } else if (!fn->fileNode(entry)) {
                fn->replaceSubtree(
                    nullptr, std::make_unique<FileNode>(entry, Node::fileTypeForFileName(entry)));
            }
        }
        QList<Node *> toRemove;
        auto filter = [&entries, &toRemove](Node *n) {
            if (!entries.contains(n->filePath()))
                toRemove << n;
        };
        fn->forEachFileNode(filter);
        fn->forEachFolderNode(filter);
        for (auto n : toRemove)
            fn->replaceSubtree(n, nullptr);
    } else {
        scan(directory);
    }
}

void WorkspaceBuildSystem::scan(const FilePath &path)
{
    m_scanQueue << path;
    scanNext();
}

void WorkspaceBuildSystem::scanNext()
{
    if (m_scanQueue.isEmpty()) {
        qCDebug(wsbs) << "Scan done.";
        m_parseGuard.markAsSuccess();
        m_parseGuard = {};

        emitBuildSystemUpdated();
    } else {
        if (!m_parseGuard.guardsProject())
            m_parseGuard = guardParsingRun();
        if (m_scanner.isFinished()) {
            const FilePath next = m_scanQueue.first();
            qCDebug(wsbs) << "Start scanning" << next;
            m_scanner.asyncScanForFiles(next);
        }
    }
}

bool WorkspaceBuildSystem::isFiltered(const FilePath &path, QList<IVersionControl *> versionControls) const
{
    const bool explicitlyExcluded = Utils::anyOf(
        m_filters,
        [path](const QRegularExpression &filter) {
            return filter.match(path.path()).hasMatch();
        });
    if (explicitlyExcluded)
        return true;
    return Utils::anyOf(versionControls, [path](const IVersionControl *vc) {
        return vc->isVcsFileOrDirectory(path);
    });
}

class WorkspaceRunConfiguration : public RunConfiguration
{
public:
    WorkspaceRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        hint.setText(Tr::tr("Clone the configuration to change it. Or, make the changes in "
                            "the .qtcreator/project.json file."));

        const BuildTargetInfo bti = buildTargetInfo();
        executable.setLabelText(Tr::tr("Executable:"));
        executable.setValue(bti.targetFilePath);
        executable.setSettingsKey("Workspace.RunConfiguration.Executable");

        auto argumentsAsString = [this]() {
            return CommandLine{
                "", buildTargetInfo().additionalData.toMap()["arguments"].toStringList()}
                .arguments();
        };

        arguments.setLabelText(Tr::tr("Arguments:"));
        arguments.setMacroExpander(macroExpander());
        arguments.setArguments(argumentsAsString());
        arguments.setSettingsKey("Workspace.RunConfiguration.Arguments");

        workingDirectory.setLabelText(Tr::tr("Working directory:"));
        workingDirectory.setDefaultWorkingDirectory(bti.workingDirectory);
        workingDirectory.setSettingsKey("Workspace.RunConfiguration.WorkingDirectory");

        setCommandLineGetter([this] {
            return CommandLine(executable.effectiveBinary(),
                               arguments.arguments(),
                               CommandLine::Raw);
        });

        setUpdater([this, argumentsAsString] {
            if (enabled.value()) // skip the update for cloned run configurations
                return;
            const BuildTargetInfo bti = buildTargetInfo();
            executable.setValue(bti.targetFilePath);
            arguments.setArguments(argumentsAsString());
            workingDirectory.setDefaultWorkingDirectory(bti.workingDirectory);
        });

        auto enabledUpdater = [this] { setEnabled(enabled.value()); };
        connect(&enabled, &BaseAspect::changed, this, enabledUpdater);
        connect(this, &AspectContainer::fromMapFinished, this, enabledUpdater);
        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
        enabledUpdater();
        enabled.setSettingsKey("Workspace.RunConfiguration.Enabled");
    }

    RunConfiguration *clone(Target *parent) override
    {
        RunConfiguration *result = RunConfiguration::clone(parent);
        dynamic_cast<WorkspaceRunConfiguration *>(result)->enabled.setValue(true);
        return result;
    }

    TextDisplay hint{this};
    FilePathAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDirectory{this};
    BoolAspect enabled{this};
};

class WorkspaceProjectRunConfigurationFactory : public RunConfigurationFactory
{
public:
    WorkspaceProjectRunConfigurationFactory()
    {
        registerRunConfiguration<WorkspaceRunConfiguration>(
            Id(WORKSPACE_PROJECT_RUNCONFIG_ID));
        addSupportedProjectType(WORKSPACE_PROJECT_ID);
    }
};

class WorkspaceProjectRunWorkerFactory : public RunWorkerFactory
{
public:
    WorkspaceProjectRunWorkerFactory()
    {
        setProduct<SimpleTargetRunner>();
        addSupportedRunMode(Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(WORKSPACE_PROJECT_RUNCONFIG_ID);
    }
};

class WorkspaceBuildConfiguration : public BuildConfiguration
{
    Q_OBJECT
public:
    WorkspaceBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        setInitializer([this](const BuildInfo &info) {
            const QVariantMap extraInfos = info.extraInfo.toMap();
            if (extraInfos.empty())
                return;
            BuildStepList *steps = buildSteps();
            for (const QVariant &buildStep : extraInfos["steps"].toList()) {
                const QVariantMap bs = buildStep.toMap();
                auto step = new Internal::ProcessStep(steps, Constants::CUSTOM_PROCESS_STEP);
                step->setCommand(FilePath::fromUserInput(bs["executable"].toString()));
                step->setArguments(bs["arguments"].toStringList());
                FilePath wd = FilePath::fromUserInput(bs["workingDirectory"].toString());
                if (wd.isEmpty())
                    wd = "%{ActiveProject:BuildConfig:Path}";
                else if (wd.isRelativePath())
                    wd = project()->projectDirectory().resolvePath(wd);
                step->setWorkingDirectory(wd);
                steps->appendStep(step);
            }
            initializeExtraInfo(extraInfos);
        });
        setBuildDirectoryHistoryCompleter("Workspace.BuildDir.History");
        setConfigWidgetDisplayName(Tr::tr("Workspace Manager"));
    }

    void initializeExtraInfo(const QVariantMap &extraInfos)
    {
        resetExtraInfo();
        if (extraInfos["forSetup"].toBool()) {
            originalExtraInfo = extraInfos;
            setEnabled(false);
            buildInfoResetConnection = connect(
                this, &BaseAspect::changed, this, &WorkspaceBuildConfiguration::resetExtraInfo);
            for (BuildStep *step : buildSteps()->steps())
                step->setEnabled(false);
        }
    }

    void fromMap(const Utils::Store &map) override
    {
        BuildConfiguration::fromMap(map);
        initializeExtraInfo(mapFromStore(storeFromVariant(map.value("extraInfo"))));
    }

    void toMap(Utils::Store &map) const override
    {
        BuildConfiguration::toMap(map);
        if (originalExtraInfo)
            map.insert("extraInfo", *originalExtraInfo);
    }

    BuildConfiguration *clone(Target *target) const override
    {
        auto clone = BuildConfiguration::clone(target);
        if (auto bc = qobject_cast<WorkspaceBuildConfiguration *>(clone); QTC_GUARD(bc))
            bc->resetExtraInfo();
        return clone;
    }

    void resetExtraInfo()
    {
        originalExtraInfo.reset();
        disconnect(buildInfoResetConnection);
        setEnabled(true);
        for (auto step : buildSteps()->steps())
            step->setEnabled(true);
    }

    std::optional<QVariantMap> originalExtraInfo;
    QMetaObject::Connection buildInfoResetConnection;
};

class WorkspaceBuildConfigurationFactory : public BuildConfigurationFactory
{
public:
    static WorkspaceBuildConfigurationFactory *m_instance;

    WorkspaceBuildConfigurationFactory()
    {
        QTC_CHECK(m_instance == nullptr);
        m_instance = this;
        registerBuildConfiguration<WorkspaceBuildConfiguration>
                ("WorkspaceProject.BuildConfiguration");

        setSupportedProjectType(WORKSPACE_PROJECT_ID);
        setSupportedProjectMimeTypeName(WORKSPACE_MIMETYPE);

        setBuildGenerator([this](const Kit *, const FilePath &projectPath, bool forSetup) {
            QList<BuildInfo> result = parseBuildConfigurations(projectPath, forSetup);
            if (!forSetup) {
                BuildInfo info;
                info.factory = this;
                info.typeName = ::ProjectExplorer::Tr::tr("Build");
                info.buildDirectory = projectPath.parentDir().parentDir().pathAppended("build");
                info.displayName = ::ProjectExplorer::Tr::tr("Default");
                result << info;
            }
            return result;
        });
    }

    static QList<BuildInfo> parseBuildConfigurations(const FilePath &projectPath, bool forSetup = false)
    {
        const QJsonObject json = projectDefinition(projectPath).value_or(QJsonObject());
        const QJsonArray buildConfigs = json.value("build.configuration").toArray();
        QList<BuildInfo> buildInfos;
        for (const QJsonValue &buildConfig : buildConfigs) {
            QTC_ASSERT(buildConfig.isObject(), continue);

            BuildInfo buildInfo;
            const QJsonObject buildConfigObject = buildConfig.toObject();
            buildInfo.displayName = buildConfigObject["name"].toString();
            if (buildInfo.displayName.isEmpty())
                continue;
            buildInfo.typeName = buildInfo.displayName;
            buildInfo.factory = m_instance;
            buildInfo.buildDirectory = FilePath::fromUserInput(
                buildConfigObject["buildDirectory"].toString());
            if (buildInfo.buildDirectory.isRelativePath()) {
                buildInfo.buildDirectory = projectPath.parentDir().parentDir().resolvePath(
                    buildInfo.buildDirectory);
            }

            QVariantList buildSteps;
            for (const QJsonValue &step : buildConfigObject["steps"].toArray()) {
                if (step.isObject() && step.toObject().contains("executable"))
                    buildSteps.append(step.toObject().toVariantMap());
            }
            if (buildSteps.isEmpty())
                continue;
            QVariantMap extraInfo = buildConfigObject.toVariantMap();
            extraInfo["forSetup"] = forSetup;
            buildInfo.extraInfo = extraInfo;

            buildInfos.append(buildInfo);
        }
        return buildInfos;
    }
};

WorkspaceBuildConfigurationFactory *WorkspaceBuildConfigurationFactory::m_instance = nullptr;

class WorkspaceProject : public Project
{
    Q_OBJECT
public:
    WorkspaceProject(const FilePath file)
    : Project(FOLDER_MIMETYPE, file.isDir() ? file / ".qtcreator" / "project.json" : file)
    {
        QTC_CHECK(projectFilePath().absolutePath().ensureWritableDir());
        if (!projectFilePath().exists() && QTC_GUARD(projectFilePath().ensureExistingFile())) {
            QJsonObject projectJson;
            projectJson.insert("$schema", "https://download.qt.io/official_releases/qtcreator/latest/installer_source/jsonschemas/project.json");
            projectJson.insert(FILES_EXCLUDE_KEY, QJsonArray{QJsonValue(".qtcreator/project.json.user")});
            projectFilePath().writeFileContents(QJsonDocument(projectJson).toJson());
        }

        setId(WORKSPACE_PROJECT_ID);
        setDisplayName(projectDirectory().fileName());
        setBuildSystemCreator<WorkspaceBuildSystem>();

        connect(this, &Project::projectFileIsDirty, this, &WorkspaceProject::updateBuildConfigurations);
    }

    void updateBuildConfigurations()
    {
        qCDebug(wsp) << "Updating build configurations for" << displayName();
        const QList<BuildInfo> buildInfos
            = WorkspaceBuildConfigurationFactory::parseBuildConfigurations(projectFilePath(), true);
        for (Target *target : targets()) {
            qCDebug(wsp) << "Updating build configurations for target" << target->displayName();
            QList<BuildInfo> toAdd = buildInfos;
            QString removedActiveBuildConfiguration;
            for (BuildConfiguration *bc : target->buildConfigurations()) {
                auto *wbc = qobject_cast<WorkspaceBuildConfiguration *>(bc);
                if (!wbc)
                    continue;
                if (std::optional<QVariantMap> extraInfo = wbc->originalExtraInfo) {
                    // remove the buildConfiguration if it is unchanged from the project file
                    auto equalExtraInfo = [&extraInfo](const BuildInfo &info) {
                        return info.extraInfo == *extraInfo;
                    };
                    if (toAdd.removeIf(equalExtraInfo) == 0) {
                        qCDebug(wsp) << " Removing build configuration" << wbc->displayName();
                        if (target->activeBuildConfiguration() == wbc)
                            removedActiveBuildConfiguration = wbc->displayName();
                        target->removeBuildConfiguration(bc);
                    }
                }
            }
            for (const BuildInfo &buildInfo : toAdd) {
                if (BuildConfiguration *bc = buildInfo.factory->create(target, buildInfo)) {
                    qCDebug(wsp) << " Adding build configuration" << bc->displayName();
                    target->addBuildConfiguration(bc);
                    if (!removedActiveBuildConfiguration.isEmpty()
                        && (bc->displayName() == removedActiveBuildConfiguration
                            || toAdd.size() == 1)) {
                        target->setActiveBuildConfiguration(bc, SetActive::NoCascade);
                    }
                }
            }
        }
    }

    FilePath projectDirectory() const override
    {
        return Project::projectDirectory().parentDir();
    }

    void saveProjectDefinition(const QJsonObject &json)
    {
        Utils::FileSaver saver(projectFilePath());
        saver.write(QJsonDocument(json).toJson());
        saver.finalize();
    }

    void excludePath(const FilePath &path)
    {
        QTC_ASSERT(projectFilePath().exists(), return);
        if (expected_str<QJsonObject> json = projectDefinition(projectFilePath())) {
            QJsonArray excludes = (*json)[FILES_EXCLUDE_KEY].toArray();
            const QString relative = path.relativePathFrom(projectDirectory()).path();
            if (excludes.contains(relative))
                return;
            excludes << relative;
            json->insert(FILES_EXCLUDE_KEY, excludes);
            saveProjectDefinition(*json);
        }
    }

    void excludeNode(Node *node)
    {
        node->setEnabled(false);
        if (auto fileNode = node->asFileNode()) {
            excludePath(fileNode->path());
        } else if (auto folderNode = node->asFolderNode()) {
            folderNode->forEachNode([](Node *node) { node->setEnabled(false); });
            excludePath(folderNode->path());
        }
    }
};

void setupWorkspaceProject(QObject *guard)
{
    ProjectManager::registerProjectType<WorkspaceProject>(FOLDER_MIMETYPE);
    ProjectManager::registerProjectType<WorkspaceProject>(WORKSPACE_MIMETYPE);

    QAction *excludeAction = nullptr;
    ActionBuilder(guard, EXCLUDE_ACTION_ID)
        .setContext(WORKSPACE_PROJECT_ID)
        .setText(Tr::tr("Exclude from Project"))
        .addToContainer(Constants::M_FOLDERCONTEXT, Constants::G_FOLDER_OTHER)
        .addToContainer(Constants::M_FILECONTEXT, Constants::G_FILE_OTHER)
        .bindContextAction(&excludeAction)
        .setCommandAttribute(Command::CA_Hide)
        .addOnTriggered(guard, [] {
            Node *node = ProjectTree::currentNode();
            QTC_ASSERT(node, return);
            const auto project = qobject_cast<WorkspaceProject *>(node->getProject());
            QTC_ASSERT(project, return);
            project->excludeNode(node);
        });

    QAction *rescanAction = nullptr;
    ActionBuilder(guard, RESCAN_ACTION_ID)
        .setContext(WORKSPACE_PROJECT_ID)
        .setText(Tr::tr("Rescan Workspace"))
        .addToContainer(Constants::M_PROJECTCONTEXT, Constants::G_PROJECT_REBUILD)
        .bindContextAction(&rescanAction)
        .setCommandAttribute(Command::CA_Hide)
        .addOnTriggered(guard, [] {
            Node *node = ProjectTree::currentNode();
            QTC_ASSERT(node, return);
            const auto project = qobject_cast<WorkspaceProject *>(node->getProject());
            QTC_ASSERT(project, return);
            if (auto target = project->activeTarget()) {
                if (auto buildSystem = dynamic_cast<WorkspaceBuildSystem *>(target->buildSystem()))
                    buildSystem->reparse(true);
            }
        });

    QObject::connect(
        ProjectTree::instance(),
        &ProjectTree::aboutToShowContextMenu,
        ProjectExplorerPlugin::instance(),
        [excludeAction, rescanAction](Node *node) {
            const bool visible = node && qobject_cast<WorkspaceProject *>(node->getProject());
            excludeAction->setVisible(visible);
            rescanAction->setVisible(visible);
            if (visible) {
                excludeAction->setEnabled(node->isEnabled());
                bool enableRescan = false;
                if (Project *project = node->getProject()) {
                    if (Target *target = project->activeTarget()) {
                        if (BuildSystem *buildSystem = target->buildSystem())
                            enableRescan = !buildSystem->isParsing();
                    }
                }
                rescanAction->setEnabled(enableRescan);
            }
        });

    static WorkspaceProjectRunConfigurationFactory theRunConfigurationFactory;
    static WorkspaceProjectRunWorkerFactory theRunWorkerFactory;
    static WorkspaceBuildConfigurationFactory theBuildConfigurationFactory;
}

} // namespace ProjectExplorer

#include "workspaceproject.moc"
