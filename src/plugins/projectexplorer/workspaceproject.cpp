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

#include <QElapsedTimer>
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

const Result<QJsonObject> projectDefinition(const FilePath &path)
{
    if (auto fileContents = path.fileContents())
        return QJsonDocument::fromJson(*fileContents).object();
    return {};
}

static QFlags<QDir::Filter> workspaceDirFilter = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden;

class WorkspaceBuildSystem final : public BuildSystem
{
public:
    WorkspaceBuildSystem(BuildConfiguration *bc);
    ~WorkspaceBuildSystem();

    static QString name() { return "Workspace"; }

    void reparse(bool force);
    void triggerParsing() final;

    bool addFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded) final;
    RemovedFilesFromProject removeFiles(Node *context, const FilePaths &filePaths, FilePaths *notRemoved) final;
    bool deleteFiles(Node *context, const FilePaths &filePaths) final;
    bool renameFiles(Node *context, const FilePairs &filesToRename, FilePaths *notRenamed) final;
    bool supportsAction(Node *context, ProjectAction action, const Node *node) const final;

    void handleDirectoryChanged(const FilePath &directory);

    void scan(const FilePath &path);

private:
    ParseGuard m_parseGuard;
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

WorkspaceBuildSystem::WorkspaceBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
{
    connect(project(), &Project::projectFileIsDirty, this, &BuildSystem::requestDelayedParse);
    requestDelayedParse();
}

void WorkspaceProject::setupScanner()
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
            qCDebug(wsbs) << "Added and collected nodes in" << timer.elapsed() << "ms"
                          << toWatch.size() << "dirs";
            m_watcher->addDirectories(toWatch, FileSystemWatcher::WatchAllChanges);
            qCDebug(wsbs) << "Added and and collected and watched nodes in" << timer.elapsed()
                          << "ms";
        };

        if (scannedDir == projectDirectory()) {
            qCDebug(wsbs) << "Finished scanning new root" << scannedDir;
            auto root = std::make_unique<ProjectNode>(scannedDir);
            root->setDisplayName(displayName());
            m_watcher.reset(new FileSystemWatcher);
            connect(
                m_watcher.get(),
                &FileSystemWatcher::directoryChanged,
                this,
                [this, w = m_watcher.get()](const FilePath &path) { handleDirectoryChanged(path); });

            addNodes(root.get());
            setRootProjectNode(std::move(root));
        } else {
            qCDebug(wsbs) << "Finished scanning subdir" << scannedDir;
            FolderNode *parent = findAvailableParent(rootProjectNode(), scannedDir);
            const FilePath relativePath = scannedDir.relativeChildPath(parent->filePath());
            const QString firstRelativeFolder = relativePath.path().left(
                relativePath.path().indexOf('/'));
            const FilePath nodePath = parent->filePath() / firstRelativeFolder;
            auto newNode = std::make_unique<FolderNode>(nodePath);
            newNode->setDisplayName(firstRelativeFolder);
            addNodes(newNode.get());
            parent->replaceSubtree(nullptr, std::move(newNode));
        }

        scanNext();
    });
    m_scanner.setDirFilter(workspaceDirFilter);
    m_scanner.setFilter([this](const MimeType &, const FilePath &filePath) {
        return anyOf(m_filters, [filePath](const QRegularExpression &filter) {
            return filter.match(filePath.path()).hasMatch();
        });
    });
}

WorkspaceBuildSystem::~WorkspaceBuildSystem()
{
    // Trigger any pending parsingFinished signals before destroying any other build system part:
    m_parseGuard = {};
}

void WorkspaceBuildSystem::reparse(bool force)
{
    if (!m_parseGuard.guardsProject())
        m_parseGuard = guardParsingRun();

    const auto project = qobject_cast<WorkspaceProject *>(this->project());

    const QList<QRegularExpression> oldFilters = project->filters();
    QList<QRegularExpression> filters;
    FilePath projectPath = project->projectDirectory();

    const QJsonObject json = projectDefinition(project->projectFilePath()).value_or(QJsonObject());
    const QJsonValue projectNameValue = json.value(PROJECT_NAME_KEY);
    if (projectNameValue.isString())
        project->setDisplayName(projectNameValue.toString());
    const QJsonArray excludesJson = json.value(FILES_EXCLUDE_KEY).toArray();
    for (const QJsonValue &excludeJson : excludesJson) {
        if (excludeJson.isString()) {
            FilePath absolute = projectPath.pathAppended(excludeJson.toString());
            filters << QRegularExpression(
                wildcardToRegularExpression(absolute.path()),
                QRegularExpression::CaseInsensitiveOption);
        }
    }
    project->setFilters(filters);

    QList<BuildTargetInfo> targetInfos;

    const QJsonArray targets = json.value("targets").toArray();
    int i = 0;
    for (const QJsonValue &target : targets) {
        i++;
        QTC_ASSERT(target.isObject(), continue);
        const QJsonObject targetObject = target.toObject();

        QJsonArray args = targetObject["arguments"].toArray();
        QStringList arguments = transform<QStringList>(args, [](const QJsonValue &arg) {
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

    if (force || oldFilters != filters)
        scan(project->projectDirectory());
    else
        emitBuildSystemUpdated();

    m_parseGuard.markAsSuccess();
    m_parseGuard = {};
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

void WorkspaceProject::handleDirectoryChanged(const FilePath &directory)
{
    ProjectNode *root = rootProjectNode();
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
        for (auto n : std::as_const(toRemove))
            fn->replaceSubtree(n, nullptr);
    } else {
        scan(directory);
    }
}

void WorkspaceBuildSystem::scan(const FilePath &path)
{
    qobject_cast<WorkspaceProject *>(project())->scan(path);
    emitBuildSystemUpdated();
}

void WorkspaceProject::scan(const FilePath &path)
{
    if (!m_scanQueue.contains(path)) {
        m_scanQueue.append(path);
        scanNext();
    }
}

void WorkspaceProject::scanNext()
{
    if (m_scanQueue.isEmpty()) {
        qCDebug(wsbs) << "Scan done.";
    } else {
        if (m_scanner.isFinished()) {
            const FilePath next = m_scanQueue.first();
            qCDebug(wsbs) << "Start scanning" << next;
            m_scanner.asyncScanForFiles(next);
        }
    }
}

bool WorkspaceProject::isFiltered(const FilePath &path, QList<IVersionControl *> versionControls) const
{
    const bool explicitlyExcluded = anyOf(m_filters, [path](const QRegularExpression &filter) {
        return filter.match(path.path()).hasMatch();
    });
    if (explicitlyExcluded)
        return true;
    return anyOf(versionControls, [path](const IVersionControl *vc) {
        return vc->isVcsFileOrDirectory(path);
    });
}

class WorkspaceRunConfiguration : public RunConfiguration
{
public:
    WorkspaceRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        enabled.setVisible(false);
        environment.setSupportForBuildEnvironment(bc);

        hint.setText(Tr::tr("Clone the configuration to change it. Or, make the changes in "
                            "the .qtcreator/project.json file."));

        const BuildTargetInfo bti = buildTargetInfo();
        executable.setLabelText(Tr::tr("Executable:"));
        executable.setExecutable(bti.targetFilePath);
        executable.setSettingsKey("Workspace.RunConfiguration.Executable");
        executable.setEnvironment(environment.environment());
        connect(&environment, &EnvironmentAspect::environmentChanged, this, [this]  {
            executable.setEnvironment(environment.environment());
        });

        auto argumentsAsString = [this]() {
            return CommandLine{
                "", buildTargetInfo().additionalData.toMap()["arguments"].toStringList()}
                .arguments();
        };

        arguments.setLabelText(Tr::tr("Arguments:"));
        arguments.setArguments(argumentsAsString());
        arguments.setSettingsKey("Workspace.RunConfiguration.Arguments");

        workingDirectory.setLabelText(Tr::tr("Working directory:"));
        if (!bti.workingDirectory.isEmpty())
            workingDirectory.setDefaultWorkingDirectory(bti.workingDirectory);
        workingDirectory.setSettingsKey("Workspace.RunConfiguration.WorkingDirectory");
        workingDirectory.setEnvironment(&environment);

        setUpdater([this, argumentsAsString] {
            if (enabled.value()) // skip the update for cloned run configurations
                return;
            const BuildTargetInfo bti = buildTargetInfo();
            executable.setExecutable(bti.targetFilePath);
            arguments.setArguments(argumentsAsString());
            if (!bti.workingDirectory.isEmpty())
                workingDirectory.setDefaultWorkingDirectory(bti.workingDirectory);
        });

        auto enabledUpdater = [this] { setEnabled(enabled.value()); };
        connect(&enabled, &BaseAspect::changed, this, enabledUpdater);
        connect(this, &AspectContainer::fromMapFinished, this, enabledUpdater);
        enabledUpdater();
        enabled.setSettingsKey("Workspace.RunConfiguration.Enabled");
    }

    RunConfiguration *clone(BuildConfiguration *bc) override
    {
        RunConfiguration *result = RunConfiguration::clone(bc);
        dynamic_cast<WorkspaceRunConfiguration *>(result)->enabled.setValue(true);
        return result;
    }

    TextDisplay hint{this};
    EnvironmentAspect environment{this};
    ExecutableAspect executable{this};
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
                step->setWorkingDirectory(wd, project()->projectDirectory());
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
        if (extraInfos.contains("env")) {
            EnvironmentItems envItems = extraInfos["env"].value<EnvironmentItems>();
            for (auto &envItem : envItems)
                envItem.value = macroExpander()->expand(envItem.value);

            setUserEnvironmentChanges(envItems);
            originalExtraInfo->remove("env");
        }
    }

    void fromMap(const Store &map) override
    {
        BuildConfiguration::fromMap(map);
        initializeExtraInfo(mapFromStore(storeFromVariant(map.value("extraInfo"))));
    }

    void toMap(Store &map) const override
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

            // We always add a default empty build configuration. This fixes several issues:
            // - If there are no build configurations defined in the project.json file,
            //   there is at least one build configuration to select.
            // - If the user changes all build configurations at once, there is always one
            //   that remains valid. The rest of the application cannot cope with there being
            //   a target without any build configurations.
            BuildInfo info;
            info.buildSystemName = WorkspaceBuildSystem::name();
            info.factory = this;
            info.typeName = Tr::tr("Empty");
            info.projectDirectory = projectPath.parentDir().parentDir();
            info.buildDirectory = info.projectDirectory.pathAppended("build");
            info.displayName = msgBuildConfigurationDefault();
            result << info;

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
            buildInfo.typeName = Tr::tr("Copy of %1").arg(buildInfo.displayName);
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

            if (buildConfigObject.contains("env")) {
                QJsonObject envObject = buildConfigObject["env"].toObject();
                EnvironmentItems envItems;

                for (QString key : envObject.keys()) {
                    QString value = envObject[key].toString();
                    EnvironmentItem::Operation op = EnvironmentItem::Operation::SetEnabled;
                    if (key.startsWith('+')) {
                        key = key.mid(1).trimmed();
                        op = EnvironmentItem::Operation::Prepend;
                    } else if (key.endsWith('+')) {
                        key = key.left(key.length() - 1).trimmed();
                        op = EnvironmentItem::Operation::Append;
                    } else if (key.startsWith('-')) {
                        key = key.mid(1).trimmed();
                        op = EnvironmentItem::Operation::Unset;
                    }

                    envItems.append(EnvironmentItem(key, value, op));
                }

                extraInfo["env"] = QVariant::fromValue(envItems);
            }

            buildInfo.extraInfo = extraInfo;

            buildInfos.append(buildInfo);
        }
        return buildInfos;
    }
};

WorkspaceBuildConfigurationFactory *WorkspaceBuildConfigurationFactory::m_instance = nullptr;

WorkspaceProject::WorkspaceProject(const FilePath &file, const QJsonObject &defaultConfiguration)
    : Project(FOLDER_MIMETYPE, file.isDir() ? file / ".qtcreator" / "project.json" : file)
{
    QTC_CHECK(projectFilePath().absolutePath().ensureWritableDir());
    if (!projectFilePath().exists() && QTC_GUARD(projectFilePath().ensureExistingFile())) {
        QJsonObject projectJson = defaultConfiguration;
        projectJson.insert("$schema", "https://download.qt.io/official_releases/qtcreator/latest/installer_source/jsonschemas/project.json");
        QJsonArray excludes = projectJson.value(FILES_EXCLUDE_KEY).toArray();
        excludes.append(".qtcreator/project.json.user");
        projectJson.insert(FILES_EXCLUDE_KEY, excludes);
        projectFilePath().writeFileContents(QJsonDocument(projectJson).toJson());
    }

    setType(WORKSPACE_PROJECT_ID);
    setDisplayName(projectDirectory().fileName());
    setBuildSystemCreator<WorkspaceBuildSystem>();

    connect(this, &Project::projectFileIsDirty, this, &WorkspaceProject::updateBuildConfigurations);

    setupScanner();
}

FilePath WorkspaceProject::projectDirectory() const
{
    return Project::projectDirectory().parentDir();
}

Project::RestoreResult WorkspaceProject::fromMap(const Store &map, QString *errorMessage)
{
    if (const RestoreResult res = Project::fromMap(map, errorMessage); res != RestoreResult::Ok)
        return res;

    if (!activeTarget()) {
        addTargetForDefaultKit();
    } else {
        // For projects created with Qt Creator < 17.
        for (Target *const t : targets()) {
            if (t->buildConfigurations().isEmpty())
                t->updateDefaultBuildConfigurations();
            QTC_CHECK(!t->buildConfigurations().isEmpty());
        }
    }
    return RestoreResult::Ok;
}

void WorkspaceProject::updateBuildConfigurations()
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

void WorkspaceProject::saveProjectDefinition(const QJsonObject &json)
{
    FileSaver saver(projectFilePath());
    saver.write(QJsonDocument(json).toJson());
    saver.finalize();
}

void WorkspaceProject::excludePath(const FilePath &path)
{
    QTC_ASSERT(projectFilePath().exists(), return);
    if (Result<QJsonObject> json = projectDefinition(projectFilePath())) {
        QJsonArray excludes = (*json)[FILES_EXCLUDE_KEY].toArray();
        const QString relative = path.relativePathFromDir(projectDirectory());
        if (excludes.contains(relative))
            return;
        excludes << relative;
        json->insert(FILES_EXCLUDE_KEY, excludes);
        saveProjectDefinition(*json);
    }
}

void WorkspaceProject::excludeNode(Node *node)
{
    node->setEnabled(false);
    if (auto fileNode = node->asFileNode()) {
        excludePath(fileNode->path());
    } else if (auto folderNode = node->asFolderNode()) {
        folderNode->forEachNode([](Node *node) { node->setEnabled(false); });
        excludePath(folderNode->path());
    }
}

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
            if (auto buildSystem = dynamic_cast<WorkspaceBuildSystem *>(project->activeBuildSystem()))
                buildSystem->reparse(true);
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
                if (BuildSystem *buildSystem = activeBuildSystem(node->getProject()))
                    enableRescan = !buildSystem->isParsing();
                rescanAction->setEnabled(enableRescan);
            }
        });

    static WorkspaceProjectRunConfigurationFactory theRunConfigurationFactory;
    static ProcessRunnerFactory theRunWorkerFactory{{WORKSPACE_PROJECT_RUNCONFIG_ID}};
    static WorkspaceBuildConfigurationFactory theBuildConfigurationFactory;
}

} // namespace ProjectExplorer

#include "workspaceproject.moc"
