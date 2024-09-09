// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "workspaceproject.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildsystem.h"
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

Q_LOGGING_CATEGORY(wsbs, "qtc.projectexplorer.workspacebuildsystem", QtWarningMsg);

const QLatin1StringView FOLDER_MIMETYPE{"inode/directory"};
const QLatin1StringView WORKSPACE_MIMETYPE{"text/x-workspace-project"};
const char WORKSPACE_PROJECT_ID[] = "ProjectExplorer.WorkspaceProject";
const char WORKSPACE_PROJECT_RUNCONFIG_ID[] = "WorkspaceProject.RunConfiguration:";

const QLatin1StringView PROJECT_NAME_KEY{"project.name"};
const QLatin1StringView FILES_EXCLUDE_KEY{"files.exclude"};
const char EXCLUDE_ACTION_ID[] = "ProjectExplorer.ExcludeFromWorkspace";
const char RESCAN_ACTION_ID[] = "ProjectExplorer.RescanWorkspace";

const expected_str<QJsonObject> projectDefinition(const Project *project)
{
    if (auto fileContents = project->projectFilePath().fileContents())
        return QJsonDocument::fromJson(*fileContents).object();
    return {};
}

static QFlags<QDir::Filter> workspaceDirFilter = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden;

class WorkspaceBuildSystem final : public BuildSystem
{
public:
    WorkspaceBuildSystem(Target *t);

    void triggerParsing() final;

    void watchFolder(const FilePath &path, const QList<IVersionControl *> &versionControls);

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

        std::vector<std::unique_ptr<FileNode>> nodePtrs
            = Utils::transform<std::vector>(m_scanner.release().allFiles, [](FileNode *fn) {
                  return std::unique_ptr<FileNode>(fn);
              });
        if (scannedDir == projectDirectory()) {
            qCDebug(wsbs) << "Finished scanning new root" << scannedDir << "found"
                          << nodePtrs.size() << "file entries";
            auto root = std::make_unique<ProjectNode>(scannedDir);
            root->setDisplayName(target()->project()->displayName());
            m_watcher.reset(new FileSystemWatcher);
            connect(
                m_watcher.get(),
                &FileSystemWatcher::directoryChanged,
                this,
                [this](const QString &path) {
                    handleDirectoryChanged(Utils::FilePath::fromPathPart(path));
                });

            root->addNestedNodes(std::move(nodePtrs));
            setRootProjectNode(std::move(root));
        } else {
            qCDebug(wsbs) << "Finished scanning subdir" << scannedDir << "found"
                          << nodePtrs.size() << "file entries";
            FolderNode *parent = findAvailableParent(project()->rootProjectNode(), scannedDir);
            const FilePath relativePath = scannedDir.relativeChildPath(parent->filePath());
            const QString firstRelativeFolder = relativePath.path().left(relativePath.path().indexOf('/'));
            const FilePath nodePath = parent->filePath() / firstRelativeFolder;
            auto newNode = std::make_unique<FolderNode>(nodePath);
            newNode->setDisplayName(firstRelativeFolder);
            newNode->addNestedNodes(std::move(nodePtrs));
            parent->replaceSubtree(nullptr, std::move(newNode));
        }
        watchFolder(scannedDir, VcsManager::versionControls());

        scanNext();
    });
    m_scanner.setDirFilter(workspaceDirFilter);
    m_scanner.setFilter([&](const Utils::MimeType &, const Utils::FilePath &filePath) {
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

void WorkspaceBuildSystem::triggerParsing()
{
    m_filters.clear();
    FilePath projectPath = project()->projectDirectory();

    const QJsonObject json = projectDefinition(project()).value_or(QJsonObject());
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

        if (!workingDirectory.isDir())
            workingDirectory = FilePath::currentWorkingPath();

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

    scan(target()->project()->projectDirectory());
}

void WorkspaceBuildSystem::watchFolder(
    const FilePath &path, const QList<IVersionControl *> &versionControls)
{
    if (!m_watcher->watchesDirectory(path)) {
        qCDebug(wsbs) << "Adding watch for " << path;
        m_watcher->addDirectory(path, FileSystemWatcher::WatchAllChanges);
    }
    for (auto entry : path.dirEntries(QDir::NoDotAndDotDot | QDir::Hidden | QDir::Dirs)) {
        if (!isFiltered(entry, versionControls))
            watchFolder(entry, versionControls);
    }
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
        executable.setReadOnly(true);
        executable.setValue(bti.targetFilePath);

        auto argumentsAsString = [this]() {
            return CommandLine{
                "", buildTargetInfo().additionalData.toMap()["arguments"].toStringList()}
                .arguments();
        };

        arguments.setLabelText(Tr::tr("Arguments:"));
        arguments.setReadOnly(true);
        arguments.setMacroExpander(macroExpander());
        arguments.setArguments(argumentsAsString());

        workingDirectory.setLabelText(Tr::tr("Working directory:"));
        workingDirectory.setReadOnly(true);
        workingDirectory.setDefaultWorkingDirectory(bti.workingDirectory);

        setCommandLineGetter([this] {
            const BuildTargetInfo bti = buildTargetInfo();
            CommandLine cmdLine{
                macroExpander()->expand(bti.targetFilePath),
                Utils::transform(
                    bti.additionalData.toMap()["arguments"].toStringList(),
                    [this](const QString &arg) { return macroExpander()->expand(arg); })};

            return cmdLine;
        });

        setUpdater([this, argumentsAsString] {
            const BuildTargetInfo bti = buildTargetInfo();
            executable.setValue(bti.targetFilePath);
            arguments.setArguments(argumentsAsString());
            workingDirectory.setDefaultWorkingDirectory(bti.workingDirectory);
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

    TextDisplay hint{this};
    FilePathAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDirectory{this};
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
public:
    WorkspaceBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        setBuildDirectoryHistoryCompleter("Workspace.BuildDir.History");
        setConfigWidgetDisplayName(Tr::tr("Workspace Manager"));

        //appendInitialBuildStep(Constants::CUSTOM_PROCESS_STEP);
    }
};

class WorkspaceBuildConfigurationFactory : public BuildConfigurationFactory
{
public:
    WorkspaceBuildConfigurationFactory()
    {
        registerBuildConfiguration<WorkspaceBuildConfiguration>
                ("WorkspaceProject.BuildConfiguration");

        setSupportedProjectType(WORKSPACE_PROJECT_ID);

        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
            BuildInfo info;
            info.typeName = ::ProjectExplorer::Tr::tr("Build");
            info.buildDirectory = projectPath.parentDir().parentDir().pathAppended("build");
            if (forSetup) {
                //: The name of the build configuration created by default for a workspace project.
                info.displayName = ::ProjectExplorer::Tr::tr("Default");
            }
            return QList<BuildInfo>{info};
        });
    }
};

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
        if (expected_str<QJsonObject> json = projectDefinition(this)) {
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
                if (target->buildSystem())
                    target->buildSystem()->triggerParsing();
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
