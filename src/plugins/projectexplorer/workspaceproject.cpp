// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "workspaceproject.h"

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

#include <utils/algorithm.h>
#include <utils/stringutils.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const QLatin1StringView FOLDER_MIMETYPE{"inode/directory"};
const QLatin1StringView WORKSPACE_MIMETYPE{"text/x-workspace-project"};
const QLatin1StringView WORKSPACE_PROJECT_ID{"ProjectExplorer.WorkspaceProject"};
const QLatin1StringView WORKSPACE_PROJECT_RUNCONFIG_ID{"WorkspaceProject.RunConfiguration:"};

const QLatin1StringView PROJECT_NAME_KEY{"project.name"};
const QLatin1StringView FILES_EXCLUDE_KEY{"files.exclude"};
const QLatin1StringView EXCLUDE_ACTION_ID{"ProjectExplorer.ExcludeFromWorkspace"};


using namespace Utils;
using namespace Core;

namespace ProjectExplorer {

const expected_str<QJsonObject> projectDefinition(const Project *project)
{
    if (auto fileContents = project->projectFilePath().fileContents())
        return QJsonDocument::fromJson(*fileContents).object();
    return {};
}

static bool checkEnabled(FolderNode *fn)
{
    if (fn->findChildFileNode([](FileNode *fn) { return fn->isEnabled(); }))
        return true;

    if (fn->findChildFolderNode([](FolderNode *fn) { return checkEnabled(fn); }))
        return true;

    fn->setEnabled(false);
    return false;
}

class WorkspaceBuildSystem : public BuildSystem
{
public:
    WorkspaceBuildSystem(Target *t)
        :BuildSystem(t)
    {
        connect(&m_scanner, &TreeScanner::finished, this, [this] {
            auto root = std::make_unique<ProjectNode>(projectDirectory());
            root->setDisplayName(target()->project()->displayName());
            std::vector<std::unique_ptr<FileNode>> nodePtrs
                = Utils::transform<std::vector>(m_scanner.release().allFiles, [this](FileNode *fn) {
                      fn->setEnabled(!Utils::anyOf(
                          m_filters, [path = fn->path().path()](const QRegularExpression &filter) {
                              return filter.match(path).hasMatch();
                          }));
                      return std::unique_ptr<FileNode>(fn);
                  });
            root->addNestedNodes(std::move(nodePtrs));
            root->forEachFolderNode(&checkEnabled);
            setRootProjectNode(std::move(root));

            m_parseGuard.markAsSuccess();
            m_parseGuard = {};

            emitBuildSystemUpdated();
        });
        m_scanner.setDirFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

        connect(target()->project(),
                &Project::projectFileIsDirty,
                this,
                &BuildSystem::requestDelayedParse);

        requestDelayedParse();
    }

    void triggerParsing() final
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
                if (absolute.isDir())
                    absolute = absolute.pathAppended("*");
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

        m_parseGuard = guardParsingRun();
        m_scanner.asyncScanForFiles(target()->project()->projectDirectory());
    }

    QString name() const final { return QLatin1String("Workspace"); }

private:
    QList<QRegularExpression> m_filters;
    ParseGuard m_parseGuard;
    TreeScanner m_scanner;
};

class WorkspaceRunConfiguration : public RunConfiguration
{
public:
    WorkspaceRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        hint.setText(
            Tr::tr("You can edit this configuration inside the .qtcreator/project.json file."));

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
            Id::fromString(WORKSPACE_PROJECT_RUNCONFIG_ID));
        addSupportedProjectType(Id::fromString(WORKSPACE_PROJECT_ID));
    }
};

class WorkspaceProjectRunWorkerFactory : public RunWorkerFactory
{
public:
    WorkspaceProjectRunWorkerFactory()
    {
        setProduct<SimpleTargetRunner>();
        addSupportedRunMode(Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Id::fromString(WORKSPACE_PROJECT_RUNCONFIG_ID));
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

        setId(Id::fromString(WORKSPACE_PROJECT_ID));
        setDisplayName(projectDirectory().fileName());
        setBuildSystemCreator([](Target *t) { return new WorkspaceBuildSystem(t); });
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

    QObject::connect(
        ProjectTree::instance(),
        &ProjectTree::aboutToShowContextMenu,
        ProjectExplorerPlugin::instance(),
        [](Node *node) {
            const bool enabled = node && node->isEnabled()
                                 && qobject_cast<WorkspaceProject *>(node->getProject());
            ActionManager::command(Id::fromString(EXCLUDE_ACTION_ID))->action()->setEnabled(enabled);
        });

    ActionBuilder excludeAction(guard, Id::fromString(EXCLUDE_ACTION_ID));
    excludeAction.setContext(Id::fromString(WORKSPACE_PROJECT_ID));
    excludeAction.setText(Tr::tr("Exclude from Project"));
    excludeAction.addToContainer(Constants::M_FOLDERCONTEXT, Constants::G_FOLDER_OTHER);
    excludeAction.addToContainer(Constants::M_FILECONTEXT, Constants::G_FILE_OTHER);
    excludeAction.addOnTriggered([] {
        Node *node = ProjectTree::currentNode();
        QTC_ASSERT(node, return);
        const auto project = qobject_cast<WorkspaceProject *>(node->getProject());
        QTC_ASSERT(project, return);
        project->excludeNode(node);
    });

    static WorkspaceProjectRunConfigurationFactory theRunConfigurationFactory;
    static WorkspaceProjectRunWorkerFactory theRunWorkerFactory;
}

} // namespace ProjectExplorer

#include "workspaceproject.moc"
