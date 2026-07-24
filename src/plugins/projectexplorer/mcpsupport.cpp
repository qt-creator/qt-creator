// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "buildconfiguration.h"
#include "buildmanager.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/idevice.h"
#include "devicesupport/idevicefactory.h"
#include "devicesupport/sshparameters.h"
#include "editorconfiguration.h"
#include "issuesmanager.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "runconfiguration.h"
#include "runcontrol.h"
#include "target.h"
#include "task.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/filesearch.h>
#include <utils/globaltasktree.h>
#include <utils/id.h>
#include <utils/mimeconstants.h>
#include <utils/result.h>
#include <utils/shutdownguard.h>

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMap>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>

using namespace Utils;

static Q_LOGGING_CATEGORY(mcpDevices, "qtc.projectexplorer.mcp", QtWarningMsg)

namespace ProjectExplorer::Internal {

static QList<Project *> projectsForName(const QString &name)
{
    return Utils::filtered(ProjectManager::projects(), Utils::equal(&Project::displayName, name));
}

// Returns the version control branch/topic for the given project directory, or an empty
// string when the project is not under a recognised version control system.
static QString currentBranchForProject(const Project *p)
{
    if (!p)
        return {};
    const FilePath dir = p->projectDirectory();
    if (dir.isEmpty())
        return {};
    if (Core::IVersionControl *vcs = Core::VcsManager::findVersionControlForDirectory(dir, nullptr))
        return vcs->vcsTopic(dir);
    return {};
}

static QString projectTypeFromMimeType(const QString &mimeType)
{
    if (mimeType == Utils::Constants::CMAKE_PROJECT_MIMETYPE)
        return "cmake";
    if (mimeType == Utils::Constants::PROFILE_MIMETYPE)
        return "qmake";
    if (mimeType == Utils::Constants::QBS_MIMETYPE)
        return "qbs";
    if (mimeType == Utils::Constants::QMLPROJECT_MIMETYPE)
        return "qmlproject";
    return mimeType; // fall back to raw mime type for unknown project kinds
}

// Returns a {name, path, branch, type} JSON object for a single project.  Used by
// list_projects (which also injects is_active) and by resolveProjects (which
// populates the candidates array for ambiguous-name errors).
static QJsonObject projectInfoObject(const Project *p)
{
    if (!p)
        return {};
    return {
        {"name", p->displayName()},
        {"path", p->projectFilePath().toUserOutput()},
        {"branch", currentBranchForProject(p)},
        {"type", projectTypeFromMimeType(p->mimeType())},
    };
}

struct ResolvedProjects
{
    QList<Project *> projects;
    QJsonArray candidates; // [{name, path, branch}] for every project that name-matched
};

// Dual-key (projectName, projectPath) lookup.  When projectPath is supplied it
// is authoritative — paths are always unique so projects.size() ≤ 1.  When
// only projectName is supplied, multiple loaded projects may share it (e.g.
// the same project open in two Git worktrees); callers detect this via
// projects.size() > 1 and should return an "ambiguous_name" error with the
// candidates list so the AI can pass projectPath to disambiguate.
static ResolvedProjects resolveProjects(const QString &projectName,
                                        const QString &projectPath)
{
    ResolvedProjects result;
    for (Project *p : ProjectManager::projects()) {
        if (!p)
            continue;
        const bool nameMatches = !projectName.isEmpty() && p->displayName() == projectName;
        const bool pathMatches = !projectPath.isEmpty()
                                 && p->projectFilePath().toUserOutput() == projectPath;
        if (!projectPath.isEmpty()) {
            // Path is authoritative when supplied — never name-match alongside it.
            if (pathMatches)
                result.projects.append(p);
        } else if (nameMatches) {
            result.projects.append(p);
        }
        if (nameMatches)
            result.candidates.append(projectInfoObject(p));
    }
    return result;
}

struct ProjectResolution
{
    Project *project = nullptr; // non-null on success
    QJsonObject error;          // ready-to-return failure object when project is null
};

// Resolves the single project a tool should act on from a (projectName,
// projectPath) pair, applying the same dual-key rules and error conventions as
// set_active_project. When both are empty: defaults to the startup project if
// defaultToStartup is true (error reason "no_startup_project" when there is
// none), otherwise fails with reason "no_target". A name matching multiple
// loaded projects fails with reason "ambiguous_name" plus a candidates array.
static ProjectResolution resolveTargetProject(
    const QString &projectName, const QString &projectPath, bool defaultToStartup)
{
    ProjectResolution r;

    if (projectName.isEmpty() && projectPath.isEmpty()) {
        if (defaultToStartup) {
            r.project = ProjectManager::startupProject();
            if (!r.project)
                r.error = {{"success", false}, {"reason", "no_startup_project"},
                           {"message", "No startup project. Pass project_name or project_path."}};
        } else {
            r.error = {{"success", false}, {"reason", "no_target"},
                       {"message", "Pass project_name or project_path to identify the target "
                                   "project."}};
        }
        return r;
    }

    const ResolvedProjects resolved = resolveProjects(projectName, projectPath);
    if (resolved.projects.isEmpty()) {
        r.error = {
            {"success", false},
            {"reason", projectPath.isEmpty() ? "project_not_loaded" : "path_not_loaded"},
            {"message",
             projectPath.isEmpty()
                 ? QString("No project named '%1' is currently loaded.").arg(projectName)
                 : QString("No project at path '%1' is currently loaded.").arg(projectPath)},
            {"candidates", resolved.candidates}};
        return r;
    }
    if (resolved.projects.size() > 1) {
        r.error = {
            {"success", false},
            {"reason", "ambiguous_name"},
            {"message",
             QString("Multiple projects named '%1' are loaded. Pass project_path "
                     "(one of the listed candidates) to disambiguate.")
                 .arg(projectName)},
            {"candidates", resolved.candidates}};
        return r;
    }

    r.project = resolved.projects.first();
    return r;
}

static QString getBuildStatus()
{
    const auto buildProgress = BuildManager::currentProgress();
    if (buildProgress)
        return QString("Building: %1% - %2").arg(buildProgress->first).arg(buildProgress->second);
    return "Not building";
}

static QStringList findFiles(const QList<Project *> &projects, const QRegularExpression &re)
{
    QStringList result;
    for (auto project : projects) {
        const FilePaths matches = project->files([&re](const Node *n) {
            return !n->filePath().isEmpty() && re.match(n->filePath().fileName()).hasMatch();
        });
        result.append(Utils::transform(matches, &FilePath::toUserOutput));
    }
    return result;
}

static QJsonArray listProjects()
{
    QJsonArray result;
    const Project *startup = ProjectManager::startupProject();
    for (Project *project : ProjectManager::projects()) {
        QJsonObject obj = projectInfoObject(project);
        obj["is_active"] = (project == startup);
        result.append(obj);
    }
    return result;
}

static QStringList listBuildConfigs()
{
    QStringList configs;
    Project *project = ProjectManager::startupProject();
    if (!project)
        return configs;
    Target *target = project->activeTarget();
    if (!target)
        return configs;
    for (BuildConfiguration *config : target->buildConfigurations())
        configs.append(config->displayName());
    return configs;
}

static bool switchToBuildConfig(const QString &name)
{
    if (name.isEmpty())
        return false;
    Project *project = ProjectManager::startupProject();
    if (!project)
        return false;
    Target *target = project->activeTarget();
    if (!target)
        return false;
    for (BuildConfiguration *config : target->buildConfigurations()) {
        if (config->displayName() == name) {
            target->setActiveBuildConfiguration(config, SetActive::Cascade);
            return true;
        }
    }
    return false;
}

static QJsonObject getCurrentProject()
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return {};
    return {
        {"project_name", project->displayName()},
        {"project_file", project->projectFilePath().toUserOutput()},
        {"project_directory", project->projectDirectory().toUserOutput()},
    };
}

static QJsonObject kitInfoObject(const Kit *k)
{
    if (!k)
        return {};
    const DetectionSource ds = k->detectionSource();
    QJsonArray issues;
    for (const Task &t : k->validate()) {
        if (!t.summary().isEmpty())
            issues.append(t.summary());
    }
    return {
        {"name", k->displayName()},
        {"id", k->id().toString()},
        {"valid", k->isValid()},
        {"has_warning", k->hasWarning()},
        {"is_default", k == KitManager::defaultKit()},
        {"auto_detected", ds.isAutoDetected()},
        {"sdk_provided", ds.isSdkProvided()},
        {"file_system_friendly_name", k->fileSystemFriendlyName()},
        {"issues", issues},
    };
}

static QJsonArray listKits()
{
    QJsonArray result;
    for (Kit *k : KitManager::sortedKits())
        result.append(kitInfoObject(k));
    return result;
}

// Returns the kits a project is configured for (one per Target), with is_active
// marking the kit of the project's active target. Defaults to the startup
// project when neither projectName nor projectPath is supplied.
static QJsonObject projectKits(const QString &projectName, const QString &projectPath)
{
    const ProjectResolution resolution = resolveTargetProject(projectName, projectPath, true);
    if (!resolution.project)
        return resolution.error;
    Project *project = resolution.project;

    const Target *activeTarget = project->activeTarget();
    QJsonArray kits;
    for (const Target *target : project->targets()) {
        QJsonObject obj = kitInfoObject(target->kit());
        obj["is_active"] = (target == activeTarget);
        kits.append(obj);
    }

    return {
        {"success", true},
        {"reason", "ok"},
        {"message", QString("Project '%1' is configured for %2 kit(s).")
                        .arg(project->displayName())
                        .arg(kits.size())},
        {"project", projectInfoObject(project)},
        {"kits", kits}};
}

// Resolves a kit by its id first (ids are unique), then falls back to an exact
// display-name match. Returns nullptr if nothing matches.
static Kit *resolveKit(const QString &identifier)
{
    if (identifier.isEmpty())
        return nullptr;
    if (Kit *byId = KitManager::kit(Utils::Id::fromString(identifier)))
        return byId;
    return KitManager::kit([&identifier](const Kit *k) { return k->displayName() == identifier; });
}

// Adds a build target for each requested kit to a project. `kitIdentifiers` may
// hold kit ids or display names. Defaults to the startup project when neither
// projectName nor projectPath is supplied. Reports per-kit results
// (added/already_present/not_found/failed) without aborting on the first error.
static QJsonObject addKitsToProject(
    const QString &projectName, const QString &projectPath, const QStringList &kitIdentifiers)
{
    const ProjectResolution resolution = resolveTargetProject(projectName, projectPath, true);
    if (!resolution.project)
        return resolution.error;
    Project *project = resolution.project;

    if (kitIdentifiers.isEmpty())
        return {{"success", false}, {"reason", "no_kits"}, {"message", "No kits specified."}};

    QJsonArray results;
    int addedCount = 0;
    for (const QString &identifier : kitIdentifiers) {
        Kit *kit = resolveKit(identifier);
        if (!kit) {
            results.append(QJsonObject{
                {"kit", identifier}, {"status", "not_found"},
                {"message", QString("No kit matching '%1'.").arg(identifier)}});
            continue;
        }
        if (project->target(kit)) {
            results.append(QJsonObject{
                {"kit", kit->displayName()}, {"id", kit->id().toString()},
                {"status", "already_present"}});
            continue;
        }
        Target *target = project->addTargetForKit(kit);
        if (target) {
            ++addedCount;
            results.append(QJsonObject{
                {"kit", kit->displayName()}, {"id", kit->id().toString()},
                {"status", "added"}});
        } else {
            results.append(QJsonObject{
                {"kit", kit->displayName()}, {"id", kit->id().toString()},
                {"status", "failed"},
                {"message", QString("Failed to add kit '%1'.").arg(kit->displayName())}});
        }
    }

    return {
        {"success", true},
        {"reason", "ok"},
        {"message", QString("Added %1 kit(s) to project '%2'.")
                        .arg(addedCount)
                        .arg(project->displayName())},
        {"project", projectInfoObject(project)},
        {"results", results}};
}

static QJsonObject openProjectFile(const QString &path)
{
    const FilePath fp = FilePath::fromUserInput(path);
    if (fp.isEmpty())
        return {{"success", false}, {"message", "Empty project path."}};
    if (!fp.exists()) {
        return {
            {"success", false},
            {"message", QString("Path does not exist: %1").arg(fp.toUserOutput())}};
    }

    const OpenProjectResult result = ProjectExplorerPlugin::openProject(fp);

    if (!result.alreadyOpen().isEmpty()) {
        Project *p = result.alreadyOpen().first();
        return {
            {"success", true},
            {"already_open", true},
            {"message",
             QString("Project '%1' is already open.").arg(p ? p->displayName() : path)},
            {"project", p ? QJsonValue(projectInfoObject(p)) : QJsonValue()}};
    }

    if (!result) {
        return {
            {"success", false},
            {"message",
             result.errorMessage().isEmpty()
                 ? QString("Failed to open project: %1").arg(fp.toUserOutput())
                 : result.errorMessage()}};
    }

    Project *p = result.project();
    return {
        {"success", true},
        {"already_open", false},
        {"message", QString("Opened project '%1'.").arg(p ? p->displayName() : path)},
        {"project", p ? QJsonValue(projectInfoObject(p)) : QJsonValue()}};
}

static QString getCurrentBuildConfig()
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return {};
    Target *target = project->activeTarget();
    if (!target)
        return {};
    BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    return buildConfig ? buildConfig->displayName() : QString{};
}

static Result<QStringList> projectDependencies(const QString &projectName)
{
    for (Project *candidate : ProjectManager::projects()) {
        if (candidate->displayName() == projectName) {
            QStringList projects;
            for (Project *project : ProjectManager::dependencies(candidate))
                projects.append(project->displayName());
            return projects;
        }
    }
    return ResultError("No project found with name: " + projectName);
}

static QMap<QString, QSet<QString>> knownRepositoriesInProject(const QString &projectName)
{
    const FilePaths projectDirectories
        = Utils::transform(projectsForName(projectName), &Project::projectDirectory);
    if (projectDirectories.isEmpty())
        return {};
    QMap<QString, QSet<QString>> repos;
    const QList<Core::IVersionControl *> versionControls = Core::VcsManager::versionControls();
    for (const Core::IVersionControl *vcs : versionControls) {
        const FilePaths repositories = Utils::filteredUnique(Core::VcsManager::repositories(vcs));
        for (const FilePath &repo : repositories) {
            if (Utils::anyOf(projectDirectories, [repo](const FilePath &projectDir) {
                    return repo == projectDir || repo.isChildOf(projectDir);
                })) {
                repos[vcs->displayName()].insert(repo.toUserOutput());
            }
        }
    }
    return repos;
}

static QJsonArray getRunConfigurations()
{
    QJsonArray result;
    Project *project = ProjectManager::startupProject();
    if (!project)
        return result;
    Target *target = project->activeTarget();
    if (!target)
        return result;
    BuildConfiguration *bc = target->activeBuildConfiguration();
    if (!bc)
        return result;
    RunConfiguration *activeRc = target->activeRunConfiguration();
    for (RunConfiguration *rc : bc->runConfigurations()) {
        QJsonObject obj;
        obj["name"] = rc->expandedDisplayName();
        obj["active"] = (rc == activeRc);
        result.append(obj);
    }
    return result;
}

// Helper: compute FindFlags from regex/caseSensitive booleans
static FindFlags mcpFindFlags(bool regex, bool caseSensitive)
{
    FindFlags flags;
    if (regex)
        flags |= FindRegularExpression;
    if (caseSensitive)
        flags |= FindCaseSensitively;
    return flags;
}

// Search helper used by search_in_files
using McpResponseCallback = std::function<void(const QJsonObject &)>;

static void mcpFindInFiles(
    FileContainer fileContainer,
    bool regex,
    bool caseSensitive,
    const QString &pattern,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QFuture<SearchResultItems> future = Utils::findInFiles(
        pattern,
        fileContainer,
        mcpFindFlags(regex, caseSensitive),
        TextEditor::TextDocument::openedTextDocumentContents());
    Utils::onFinished(future, guard, [callback](const QFuture<SearchResultItems> &future) {
        QJsonArray resultsArray;
        for (Utils::SearchResultItems results : future.results()) {
            for (const SearchResultItem &item : results) {
                QJsonObject resultObj;
                const Text::Range range = item.mainRange();
                const QString lineText = item.lineText();
                const int startCol = range.begin.column;
                const int endCol = range.end.column;
                const QString matchedText = lineText.mid(startCol, endCol - startCol);

                resultObj["file"] = item.path().value(0, QString());
                resultObj["line"] = range.begin.line;
                resultObj["column"] = startCol + 1;
                resultObj["text"] = matchedText;
                resultsArray.append(resultObj);
            }
        }
        QJsonObject response;
        response["results"] = resultsArray;
        callback(response);
    });
}

// Replace helper used by replace_in_files
static void mcpReplace(
    FileContainer fileContainer,
    bool regex,
    bool caseSensitive,
    const QString &pattern,
    const QString &replacement,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QFuture<SearchResultItems> future = Utils::findInFiles(
        pattern,
        fileContainer,
        mcpFindFlags(regex, caseSensitive),
        TextEditor::TextDocument::openedTextDocumentContents());
    Utils::onFinished(future, guard, [callback, replacement](const QFuture<SearchResultItems> &future) {
        QJsonObject response;
        bool success = true;

        TextEditor::PlainRefactoringFileFactory changes;
        QHash<Utils::FilePath, TextEditor::RefactoringFilePtr> refactoringFiles;

        for (const SearchResultItems &results : future.results()) {
            for (const SearchResultItem &item : results) {
                Text::Range range = item.mainRange();
                if (range.begin >= range.end)
                    continue;
                const FilePath filePath = FilePath::fromUserInput(item.path().value(0));
                if (filePath.isEmpty())
                    continue;
                TextEditor::RefactoringFilePtr refactoringFile = refactoringFiles.value(filePath);
                if (!refactoringFile)
                    refactoringFile
                        = refactoringFiles.insert(filePath, changes.file(filePath)).value();
                const int start = refactoringFile->position(range.begin);
                const int end = refactoringFile->position(range.end);
                ChangeSet changeSet = refactoringFile->changeSet();
                changeSet.replace(ChangeSet::Range(start, end), replacement);
                refactoringFile->setChangeSet(changeSet);
            }
        }

        for (auto refactoringFile : refactoringFiles) {
            if (!refactoringFile->apply())
                success = false;
        }

        response["ok"] = success;
        callback(response);
    });
}

static void searchInFiles(
    const QString &filePattern,
    const std::optional<QString> &projectName,
    const QString &pattern,
    bool regex,
    bool caseSensitive,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QList<Project *> projects = projectName ? projectsForName(*projectName)
                                                  : ProjectManager::projects();

    const FilterFilesFunction filterFiles
        = Utils::filterFilesFunction({filePattern.isEmpty() ? "*" : filePattern}, {});
    const QMap<FilePath, TextEncoding> openEditorEncodings
        = TextEditor::TextDocument::openedTextDocumentEncodings();
    QMap<FilePath, TextEncoding> encodings;
    for (const Project *project : projects) {
        const EditorConfiguration *config = project->editorConfiguration();
        TextEncoding projectEncoding = config->useGlobalSettings()
                                           ? Core::EditorManager::defaultTextEncoding()
                                           : config->textEncoding();
        const FilePaths filteredFiles = filterFiles(project->files(
            Core::Find::hasFindFlag(DontFindGeneratedFiles) ? Project::SourceFiles
                                                            : Project::AllFiles));
        for (const FilePath &fileName : filteredFiles) {
            TextEncoding encoding = openEditorEncodings.value(fileName);
            if (!encoding.isValid())
                encoding = projectEncoding;
            encodings.insert(fileName, encoding);
        }
    }
    FileListContainer fileContainer(encodings.keys(), encodings.values());
    mcpFindInFiles(fileContainer, regex, caseSensitive, pattern, guard, callback);
}

static void replaceInFiles(
    const QString &filePattern,
    const std::optional<QString> &projectName,
    const QString &pattern,
    const QString &replacement,
    bool regex,
    bool caseSensitive,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QList<Project *> projects = projectName ? projectsForName(*projectName)
                                                  : ProjectManager::projects();

    const FilterFilesFunction filterFiles
        = Utils::filterFilesFunction({filePattern.isEmpty() ? "*" : filePattern}, {});
    const QMap<FilePath, TextEncoding> openEditorEncodings
        = TextEditor::TextDocument::openedTextDocumentEncodings();
    QMap<FilePath, TextEncoding> encodings;
    for (const Project *project : projects) {
        const EditorConfiguration *config = project->editorConfiguration();
        TextEncoding projectEncoding = config->useGlobalSettings()
                                           ? Core::EditorManager::defaultTextEncoding()
                                           : config->textEncoding();
        const FilePaths filteredFiles = filterFiles(project->files(
            Core::Find::hasFindFlag(DontFindGeneratedFiles) ? Project::SourceFiles
                                                            : Project::AllFiles));
        for (const FilePath &fileName : filteredFiles) {
            TextEncoding encoding = openEditorEncodings.value(fileName);
            if (!encoding.isValid())
                encoding = projectEncoding;
            encodings.insert(fileName, encoding);
        }
    }
    FileListContainer fileContainer(encodings.keys(), encodings.values());
    mcpReplace(fileContainer, regex, caseSensitive, pattern, replacement, guard, callback);
}

// --- Device helpers --------------------------------------------------------

static QString deviceStateString(IDevice::DeviceState state)
{
    switch (state) {
    case IDevice::DeviceReadyToUse:
        return "ready";
    case IDevice::DeviceConnected:
        return "connected";
    case IDevice::DeviceDisconnected:
        return "disconnected";
    case IDevice::DeviceStateUnknown:
        break;
    }
    return "unknown";
}

static QString hostKeyCheckingModeString(SshHostKeyCheckingMode mode)
{
    switch (mode) {
    case SshHostKeyCheckingNone:
        return "none";
    case SshHostKeyCheckingStrict:
        return "strict";
    case SshHostKeyCheckingAllowNoMatch:
        break;
    }
    return "allowNoMatch";
}

static SshHostKeyCheckingMode parseHostKeyCheckingMode(
    const QString &s, SshHostKeyCheckingMode fallback)
{
    if (s == "none")
        return SshHostKeyCheckingNone;
    if (s == "strict")
        return SshHostKeyCheckingStrict;
    if (s == "allowNoMatch")
        return SshHostKeyCheckingAllowNoMatch;
    return fallback;
}

static QJsonObject deviceToJson(const IDevice::ConstPtr &device)
{
    const SshParameters ssh = device->sshParameters();
    return QJsonObject{
        {"id", device->id().toString()},
        {"type", device->type().toString()},
        {"displayName", device->displayName()},
        {"displayType", device->displayType()},
        {"state", deviceStateString(device->deviceState())},
        {"rootPath", device->rootPath().toUrlishString()},
        {"host", ssh.host()},
        {"port", ssh.port()},
        {"userName", ssh.userName()},
        {"useKeyFile", ssh.authenticationType() == SshParameters::AuthenticationTypeSpecificKey},
        {"privateKeyFile", ssh.privateKeyFile().toUserOutput()},
        {"timeout", ssh.timeout()},
        {"hostKeyCheckingMode", hostKeyCheckingModeString(ssh.hostKeyCheckingMode())},
    };
}

// Overlays the SSH-related fields present in the JSON object onto a base parameter set,
// so callers can update individual fields without resetting the others.
static SshParameters mergeSshParameters(const SshParameters &base, const QJsonObject &params)
{
    SshParameters ssh = base;
    if (params.contains("host"))
        ssh.setHost(params.value("host").toString());
    if (params.contains("port"))
        ssh.setPort(params.value("port").toInt(ssh.port()));
    if (params.contains("userName"))
        ssh.setUserName(params.value("userName").toString());
    if (params.contains("privateKeyFile"))
        ssh.setPrivateKeyFile(FilePath::fromUserInput(params.value("privateKeyFile").toString()));
    if (params.contains("useKeyFile")) {
        ssh.setAuthenticationType(
            params.value("useKeyFile").toBool() ? SshParameters::AuthenticationTypeSpecificKey
                                                 : SshParameters::AuthenticationTypeAll);
    }
    if (params.contains("timeout"))
        ssh.setTimeout(params.value("timeout").toInt(ssh.timeout()));
    if (params.contains("hostKeyCheckingMode")) {
        ssh.setHostKeyCheckingMode(parseHostKeyCheckingMode(
            params.value("hostKeyCheckingMode").toString(), ssh.hostKeyCheckingMode()));
    }
    return ssh;
}

static void applySshParameters(const IDevice::Ptr &device, const QJsonObject &params)
{
    const SshParameters ssh = mergeSshParameters(device->sshParameters(), params);
    device->sshParametersAspectContainer().setSshParameters(ssh);
    device->sshParametersAspectContainer().apply();
}

void registerMcpTools()
{
    using namespace Mcp::Schema;
    namespace Schema = Mcp::Schema;
    using Mcp::ToolInterface;
    using Mcp::ToolRegistry;

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    static const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    using Callback = std::function<void(const QJsonObject &)>;
    using SimplifiedAsyncCallback = std::function<void(const QJsonObject &, const Callback &)>;
    static const auto wrapAsync =
        [](SimplifiedAsyncCallback asyncFunc) -> Mcp::Server::ToolInterfaceCallback {
        return [asyncFunc](
                   const Schema::CallToolRequestParams &params,
                   const ToolInterface &toolInterface) -> Utils::Result<> {
            asyncFunc(params.argumentsAsObject(), [toolInterface](QJsonObject result) {
                toolInterface.finish(CallToolResult{}.isError(false).structuredContent(result));
            });
            return ResultOk;
        };
    };

    // Persistent issues manager for all PE mcp tools
    static ProjectExplorer::IssuesManager issuesManager;

    // Slot guard for serializing concurrent build() tool calls.  Qt Creator's
    // BuildManager queues projects internally but emits a single
    // buildQueueFinished signal for the whole queue -- two concurrent MCP
    // build() tasks would both receive the first queue's verdict.  This struct
    // ensures only one MCP-initiated build task has live signal connections at
    // a time; other callers wait in the heartbeat or refuse immediately,
    // depending on the on_busy parameter.
    struct BuildSlot
    {
        bool inProgress = false;
        QPointer<Project> project;
        QElapsedTimer elapsed;
    };
    static BuildSlot buildSlot;

    // Output schema for `build`. Designed so the AI doesn't have to inspect
    // `issues` to guess the build verdict — `succeeded` is the single source
    // of truth, mirrored by the tool's TaskStatus (completed vs failed).
    const auto buildOutputSchema
        = Tool::OutputSchema{}
              .addProperty(
                  "succeeded",
                  QJsonObject{
                      {"type", "boolean"},
                      {"description",
                       "Whether the build finished without errors. Single source of truth — "
                       "use this rather than inspecting `issues` to decide success/failure."}})
              .addProperty(
                  "error_count",
                  QJsonObject{
                      {"type", "integer"},
                      {"minimum", 0},
                      {"description", "Number of error-level issues from this build."}})
              .addProperty(
                  "warning_count",
                  QJsonObject{
                      {"type", "integer"},
                      {"minimum", 0},
                      {"description", "Number of warning-level issues from this build."}})
              .addProperty(
                  "duration_ms",
                  QJsonObject{
                      {"type", "integer"},
                      {"minimum", 0},
                      {"description",
                       "Wall-clock duration in milliseconds, from buildProjects() to "
                       "buildQueueFinished."}})
              .addProperty(
                  "issues",
                  QJsonObject{
                      {"type", "array"},
                      {"description",
                       "Same shape as list_issues' issues array. Empty on successful "
                       "builds with no warnings."}})
              .addProperty(
                  "summary_text",
                  QJsonObject{
                      {"type", "string"},
                      {"description",
                       "Human-readable one-liner like 'Build succeeded in 4523 ms' or "
                       "'Build failed with 2 error(s), 3 warning(s) in 1820 ms'."}})
              .addProperty(
                  "output",
                  QJsonObject{
                      {"type", "string"},
                      {"description",
                       "Captured stdout+stderr from the build. May be truncated if the build "
                       "produced an extremely large amount of output."}})
              .addRequired("succeeded")
              .addRequired("error_count")
              .addRequired("warning_count")
              .addRequired("duration_ms")
              .addRequired("issues")
              .addRequired("summary_text")
              .addRequired("output");

    ToolRegistry::registerTool(
        Schema::Tool()
            .name("build")
            .title("Build project")
            .description(
                "Builds the chosen project (or the active startup project if no name is "
                "given) and blocks until the build finishes. Returns an explicit verdict: "
                "{succeeded, error_count, warning_count, duration_ms, issues, "
                "summary_text}. The TaskStatus mirrors the verdict — `completed` on "
                "success, `failed` on build failure. The `issues` array carries any "
                "warnings/errors recorded by the build (same shape as list_issues)."
                "\n\n"
                "Use `succeeded` to decide success/failure — don't try to infer it from "
                "the issues array. This verdict is the return value of build() itself; "
                "you don't need get_build_status to confirm it."
                "\n\n"
                "When two build() calls arrive concurrently, on_busy controls the "
                "behaviour: \"queue\" (default) waits for the in-progress build to finish "
                "before starting this one; \"refuse\" returns immediately with "
                "succeeded:false and reason:\"build_in_progress\" so the caller can "
                "decide when to retry.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"description",
                             "Name of the project to build. Pass project_path as well when "
                             "multiple loaded projects share this name."},
                            {"type", "string"}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"description",
                             "Absolute path to the project file (CMakeLists.txt, .pro, ...). "
                             "Unambiguously identifies the project when several share the "
                             "same display name."},
                            {"type", "string"}})
                    .addProperty(
                        "on_busy",
                        QJsonObject{
                            {"description",
                             "Behaviour when another MCP-initiated build is already running. "
                             "\"queue\" (default): wait for it to finish, then build. "
                             "\"refuse\": return immediately with reason \"build_in_progress\"."},
                            {"type", "string"},
                            {"enum", QJsonArray{"queue", "refuse"}}}))
            .outputSchema(buildOutputSchema)
            .annotations(
                Schema::ToolAnnotations()
                    .destructiveHint(false)
                    .idempotentHint(true)
                    .openWorldHint(false)
                    .readOnlyHint(false)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QString projectName = params.arguments()->value("project_name").toString();
            const QString projectPath = params.arguments()->value("project_path").toString();
            const QString onBusy = params.arguments()->value("on_busy").toString("queue");

            // Resolve the target the same way every other project-scoped tool
            // does: resolveTargetProject defaults to the startup project when
            // neither key is given, and returns a structured error (reason,
            // message, candidates) for the not-loaded and ambiguous-name cases.
            const ProjectResolution resolution
                = resolveTargetProject(projectName, projectPath, /*defaultToStartup=*/true);
            if (!resolution.project) {
                // Enrich the shared resolution error with build's verdict fields
                // so the response still satisfies the output schema, then surface
                // it as a tool error the AI can act on (e.g. retry with project_path).
                QJsonObject body = resolution.error;
                body.remove("success"); // build() reports via "succeeded", not "success"
                body["succeeded"] = false;
                body["error_count"] = 0;
                body["warning_count"] = 0;
                body["duration_ms"] = 0;
                body["issues"] = QJsonArray{};
                body["summary_text"] = resolution.error.value("message");
                body["output"] = QString(); // required by the output schema
                toolInterface.finish(CallToolResult{}.isError(true).structuredContent(body));
                return ResultOk;
            }

            // QPointer, not a raw pointer: the build launch is deferred into the
            // heartbeat (and may wait in the on_busy=="queue" branch), so the
            // project can be unloaded before we use it. The heartbeat null-checks.
            const QPointer<Project> targetProject = resolution.project;

            // State shared between the heartbeat, the buildQueueFinished
            // handler, and the finishFn.
            //
            // buildStarted:  has buildProjects() been called for this invocation?
            //   The heartbeat sets this once it has claimed the BuildSlot and
            //   kicked off the build, so that subsequent heartbeats skip the
            //   slot-acquisition phase.
            //
            // refused: set when on_busy=="refuse" and the slot was taken.
            //   The finishFn returns a structured build_in_progress response
            //   rather than a real build verdict.
            //
            // succeeded: pessimistic default.  Set to true only when
            //   buildQueueFinished(true) fires.  If the nested loop exits
            //   without the signal (cancellation, quit, stale wakeup), the
            //   function correctly reports failure rather than a phantom success.
            //
            // earlyError: set for terminal pre-build failures (target unloaded,
            //   or buildProjects() queued nothing so no verdict signal will ever
            //   fire). When non-empty the finishFn returns it verbatim, so the
            //   task fails fast instead of polling forever.
            struct State
            {
                bool buildStarted = false;
                bool finished = false;
                bool refused = false;
                bool succeeded = false; // pessimistic -- true only on explicit signal
                QJsonObject refusedBlockingInfo;
                qint64 refusedElapsedMs = 0;
                QJsonObject earlyError;
                QElapsedTimer timer;
            };
            auto state = std::make_shared<State>();

            struct Output
            {
                QMetaObject::Connection connection = QObject::connect(
                    BuildManager::instance(),
                    &BuildManager::outputText,
                    BuildManager::instance(),
                    [this](const QString &text) {
                        this->text += text;
                        // 100 KB cap to not explode the context size.
                        static constexpr auto maxOutputSize = 1024 * 100;
                        if (this->text.size() > maxOutputSize)
                            this->text = this->text.right(maxOutputSize);
                    });
                QString text;

                ~Output() { QObject::disconnect(connection); }

                Output() = default;
                Output(const Output &) = delete;
                Output(Output &&) = delete;
                Output &operator=(const Output &) = delete;
                Output &operator=(Output &&) = delete;
            };

            auto output = std::make_shared<Output>();

            using namespace std::chrono_literals;

            const auto buildTask = toolInterface.startTask(
                1s,
                [state, targetProject, onBusy, output](Schema::Task task) -> Schema::Task {
                    // Phase 1 -- slot acquisition (runs each heartbeat tick until
                    // the slot is free and the build has been launched).
                    if (!state->buildStarted && !state->refused) {
                        if (!targetProject) {
                            // Project was unloaded between resolution and launch.
                            state->finished = true;
                            state->earlyError = QJsonObject{
                                {"succeeded", false},
                                {"reason", "project_unloaded"},
                                {"error_count", 0},
                                {"warning_count", 0},
                                {"duration_ms", 0},
                                {"issues", QJsonArray{}},
                                {"summary_text",
                                 "Target project was unloaded before the build could start."},
                                {"output", QString()},
                            };
                            task.status(Schema::TaskStatus::failed);
                            task.statusMessage("Target project was unloaded");
                            Mcp::letTaskDieIn(task, 1min);
                            return task;
                        }
                        if (buildSlot.inProgress) {
                            // Reclaim a stale slot: if BuildManager is no longer building
                            // (e.g. shutdown, external cancel, or missed signal), don't
                            // block indefinitely.
                            if (!BuildManager::isBuilding()) {
                                buildSlot = {};
                            } else if (onBusy == QLatin1String("refuse")) {
                                state->refused = true;
                                if (buildSlot.project)
                                    state->refusedBlockingInfo = projectInfoObject(
                                        buildSlot.project);
                                state->refusedElapsedMs = buildSlot.elapsed.elapsed();
                                task.status(Schema::TaskStatus::failed);
                                task.statusMessage("Refused: another build is in progress");
                                Mcp::letTaskDieIn(task, 1min);
                                return task;
                            } else {
                                // on_busy=="queue": report wait status and come back next tick.
                                const QString blockName = buildSlot.project
                                                              ? buildSlot.project->displayName()
                                                              : QStringLiteral("?");
                                task.statusMessage(
                                    QString("Waiting for '%1' build to finish (%2 ms)...")
                                        .arg(blockName)
                                        .arg(buildSlot.elapsed.elapsed()));
                                return task.status(Schema::TaskStatus::working);
                            }
                        }

                        // Slot is free -- claim it and launch the build.
                        buildSlot.inProgress = true;
                        buildSlot.project = targetProject;
                        buildSlot.elapsed.start();
                        state->buildStarted = true;
                        state->timer.start();
                        // Drop anything captured while queued: the outputText
                        // connection was live during the wait, so output->text
                        // holds the blocking build's tail. Scope it to our build.
                        output->text.clear();

                        // Connect BEFORE buildProjects() to avoid losing a
                        // synchronously-emitted buildQueueFinished (e.g. when
                        // everything is already up-to-date and the queue drains in
                        // one tick).  SingleShotConnection ensures exactly one
                        // verdict per invocation.
                        const QMetaObject::Connection finishedConn = QObject::connect(
                            BuildManager::instance(),
                            &BuildManager::buildQueueFinished,
                            BuildManager::instance(),
                            [state](bool success) {
                                state->succeeded = success;
                                state->finished = true;
                                buildSlot = {}; // release immediately so queued callers unblock
                            },
                            Qt::SingleShotConnection);

                        // buildProjects() returns the number of build steps queued.
                        // <= 0 means nothing started (e.g. no build configuration for
                        // the kit): buildQueueFinished will never fire, so tear down
                        // the pending connection, release the slot, and fail fast
                        // instead of polling forever and leaking the slot.
                        if (BuildManager::buildProjects({targetProject}, ConfigSelection::Active)
                            <= 0) {
                            QObject::disconnect(finishedConn);
                            buildSlot = {};
                            state->finished = true;
                            state->earlyError = QJsonObject{
                                {"succeeded", false},
                                {"reason", "build_failed_to_start"},
                                {"error_count", 0},
                                {"warning_count", 0},
                                {"duration_ms", 0},
                                {"issues", QJsonArray{}},
                                {"summary_text",
                                 output->text.isEmpty()
                                     ? QStringLiteral("Build failed to start. Check that the "
                                                      "project is configured for this kit.")
                                     : QString("Build failed to start.\n%1").arg(output->text)},
                                {"output", output->text},
                            };
                            task.status(Schema::TaskStatus::failed);
                            task.statusMessage("Build failed to start");
                            Mcp::letTaskDieIn(task, 1min);
                            return task;
                        }
                    }

                    // Phase 2 -- build is running; poll for completion.
                    if (state->finished) {
                        task.status(state->succeeded ? Schema::TaskStatus::completed
                                                     : Schema::TaskStatus::failed);
                        task.statusMessage(QString::fromLatin1(
                            state->succeeded ? "Build succeeded" : "Build failed"));
                        Mcp::letTaskDieIn(task, 1min);
                        return task;
                    }
                    if (auto progress = BuildManager::currentProgress()) {
                        task.statusMessage(
                            QString("%1 (%2%)").arg(progress->second).arg(progress->first));
                    }
                    return task.status(Schema::TaskStatus::working);
                },
                [state, output]() -> Utils::Result<Schema::CallToolResult> {
                    if (!state->earlyError.isEmpty()) {
                        // Terminal pre-build failure (target unloaded, or nothing
                        // queued): no verdict signal will ever come.
                        return CallToolResult{}.structuredContent(state->earlyError).isError(true);
                    }
                    if (state->refused) {
                        const QString blockName
                            = state->refusedBlockingInfo.value("name").toString("?");
                        return CallToolResult{}
                            .structuredContent(QJsonObject{
                                {"succeeded", false},
                                {"reason", "build_in_progress"},
                                {"blocking_project", state->refusedBlockingInfo},
                                {"blocking_running_for_ms", state->refusedElapsedMs},
                                {"error_count", 0},
                                {"warning_count", 0},
                                {"duration_ms", 0},
                                {"issues", QJsonArray{}},
                                {"summary_text",
                                 QString("Build in progress for '%1' (%2 ms). Pass "
                                         "on_busy:\"queue\" to wait, or retry later.")
                                     .arg(blockName)
                                     .arg(state->refusedElapsedMs)},
                                {"output", QString()}, // required by the output schema
                            })
                            .isError(true);
                    }

                    const QJsonObject issuesData = issuesManager.getCurrentIssues();
                    const QJsonObject issuesSummary = issuesData.value("summary").toObject();
                    const int errorCount = issuesSummary.value("errorCount").toInt();
                    const int warningCount = issuesSummary.value("warningCount").toInt();
                    const qint64 durationMs = state->timer.elapsed();

                    QString summaryText;
                    if (state->succeeded) {
                        summaryText = warningCount == 0
                                          ? QString("Build succeeded in %1 ms").arg(durationMs)
                                          : QString("Build succeeded with %1 warning(s) in %2 ms")
                                                .arg(warningCount)
                                                .arg(durationMs);
                    } else {
                        summaryText
                            = QString("Build failed with %1 error(s), %2 warning(s) in %3 ms")
                                  .arg(errorCount)
                                  .arg(warningCount)
                                  .arg(durationMs);
                    }

                    return CallToolResult{}
                        .structuredContent(
                            QJsonObject{
                                {"succeeded", state->succeeded},
                                {"error_count", errorCount},
                                {"warning_count", warningCount},
                                {"duration_ms", durationMs},
                                {"issues", issuesData.value("issues")},
                                {"summary_text", summaryText},
                                {"output", output->text},
                            })
                        .isError(!state->succeeded);
                },
                [state]() {
                    // Only tear down a build that is still ours. On normal
                    // completion buildQueueFinished already released the slot,
                    // and a queued build() may now hold it -- releasing here
                    // would wipe theirs and let a third caller claim it mid-build.
                    if (state->buildStarted && !state->finished) {
                        BuildManager::cancel();
                        buildSlot = {};
                    }
                },
                Mcp::progressToken(params));

            if (!buildTask) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(buildTask.error())));
                return ResultOk;
            }
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("list_issues")
            .title("List current issues (warnings and errors)")
            .description("List current issues (warnings and errors)")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(ProjectExplorer::IssuesManager::issuesSchema()),
        wrap([](const QJsonObject &) { return issuesManager.getCurrentIssues(); }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_file_issues")
            .title("List current issues for file (warnings and errors)")
            .description("List current issues for file (warnings and errors)")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file to open"}})
                    .addRequired("path"))
            .outputSchema(ProjectExplorer::IssuesManager::issuesSchema()),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            return issuesManager.getCurrentIssues(Utils::FilePath::fromUserInput(path));
        }));

    // Shared output schema for run/debug tools.
    const auto runToolOutputSchema = [&]() {
        const Tool::OutputSchema issSchema = ProjectExplorer::IssuesManager::issuesSchema();
        QJsonObject issuesField{
            {"type", "object"},
            {"description",
             "Build issues — present when the build failed; same format as list_issues"}};
        if (issSchema._properties) {
            QJsonObject props;
            for (auto it = issSchema._properties->cbegin(); it != issSchema._properties->cend();
                 ++it)
                props[it.key()] = it.value();
            issuesField["properties"] = props;
        }
        if (issSchema._required)
            issuesField["required"] = QJsonArray::fromStringList(*issSchema._required);

        return Tool::OutputSchema{}
            .addProperty(
                "output",
                QJsonObject{
                    {"type", "string"},
                    {"description", "Collected output from the run (present on success)"}})
            .addProperty(
                "exitCode",
                QJsonObject{
                    {"type", "integer"},
                    {"description",
                     "Process exit code, when the run produced one. Absent if it crashed or "
                     "terminated abnormally (e.g. a failing terminal launch)."}})
            .addProperty(
                "succeeded",
                QJsonObject{
                    {"type", "boolean"},
                    {"description", "True if the run finished with exit code 0."}})
            .addProperty("issues", issuesField);
    }();

    // Shared callback factory for run/debug tools.
    const auto makeRunCallback =
        [](Utils::Id runMode, const QString &finishedMessage) -> Mcp::Server::ToolInterfaceCallback {
        return [runMode, finishedMessage](
                   const Schema::CallToolRequestParams &params,
                   const ToolInterface &toolInterface) -> Utils::Result<> {
            const Utils::Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(runMode);
            if (!canRun) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(canRun.error())));
                return ResultOk;
            }

            struct State
            {
                QStringList output;
                bool finished = false;
                QJsonObject failureIssues;
                QPointer<RunControl> rc;
                std::optional<int> exitCode;
            };
            auto state = std::make_shared<State>();

            using namespace std::chrono_literals;

            const Utils::Result<ToolInterface::TaskProgressNotify> task = toolInterface.startTask(
                500ms,
                [state](Schema::Task t) {
                    if (state->finished)
                        Mcp::letTaskDieIn(t, 1min);
                    const bool failed = !state->failureIssues.isEmpty();
                    return t
                        .status(
                            !state->finished ? Schema::TaskStatus::working
                            : failed         ? Schema::TaskStatus::failed
                                             : Schema::TaskStatus::completed)
                        .statusMessage(
                            state->output.isEmpty() ? std::nullopt
                                                    : std::optional{state->output.last()});
                },
                [state]() -> Utils::Result<CallToolResult> {
                    if (!state->failureIssues.isEmpty())
                        return CallToolResult{}.isError(true).structuredContent(
                            QJsonObject{{"issues", state->failureIssues}});
                    QJsonObject out{{"output", state->output.join('\n')}};
                    if (state->exitCode)
                        out["exitCode"] = *state->exitCode;
                    out["succeeded"] = state->exitCode.has_value() && *state->exitCode == 0;
                    return CallToolResult{}.isError(false).structuredContent(out);
                },
                [state]() {
                    if (state->rc)
                        state->rc->initiateStop();
                },
                Mcp::progressToken(params));

            if (!task) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(task.error())));
                return ResultOk;
            }

            const ToolInterface::TaskProgressNotify notify = *task;

            auto rcStartedConn = std::make_shared<QMetaObject::Connection>();

            *rcStartedConn = QObject::connect(
                ProjectExplorerPlugin::instance(),
                &ProjectExplorerPlugin::runControlStarted,
                ProjectExplorerPlugin::instance(),
                [state, notify, rcStartedConn, runMode](RunControl *rc) {
                    if (rc->runMode() != runMode)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    state->rc = rc;
                    QObject::connect(
                        rc,
                        &RunControl::appendMessage,
                        rc,
                        [state, notify](const QString &msg, Utils::OutputFormat) {
                            const QString trimmed = msg.trimmed();
                            if (trimmed.isEmpty())
                                return;
                            state->output.append(trimmed);
                            if (notify)
                                notify(Schema::TaskStatus::working, trimmed, std::nullopt);
                        });
                });

            // Complete on stop even if the run never reached started(): a run
            // that fails during startup (e.g. a broken terminal whose stub never
            // connects) goes straight to stopped without emitting started(), so
            // runControlStarted never fires. runControlStoped fires on stop
            // regardless, so it is the reliable completion signal.
            auto rcStoppedConn = std::make_shared<QMetaObject::Connection>();
            *rcStoppedConn = QObject::connect(
                ProjectExplorerPlugin::instance(),
                &ProjectExplorerPlugin::runControlStoped,
                ProjectExplorerPlugin::instance(),
                [state, notify, rcStartedConn, rcStoppedConn, runMode, finishedMessage](
                    RunControl *rc) {
                    if (state->finished || rc->runMode() != runMode)
                        return;
                    if (state->rc && state->rc != rc)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    QObject::disconnect(*rcStoppedConn);
                    state->rc = rc;
                    state->finished = true;
                    state->exitCode = rc->lastExitCode();
                    if (notify)
                        notify(Schema::TaskStatus::completed, finishedMessage, std::nullopt);
                });

            QObject::connect(
                BuildManager::instance(),
                &BuildManager::buildQueueFinished,
                BuildManager::instance(),
                [state, notify, rcStartedConn](bool success) {
                    if (success || state->rc)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    state->finished = true;
                    state->failureIssues = issuesManager.getCurrentIssues();
                    const int errorCount = state->failureIssues.value("summary")
                                               .toObject()
                                               .value("errorCount")
                                               .toInt();
                    const QString statusMsg
                        = errorCount > 0 ? QString("Build failed with %1 error(s)").arg(errorCount)
                                         : QString("Build failed");
                    if (notify)
                        notify(Schema::TaskStatus::failed, statusMsg, std::nullopt);
                },
                Qt::SingleShotConnection);

            ProjectExplorerPlugin::runStartupProject(runMode, false);
            return ResultOk;
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("run_project")
            .title("Run project")
            .description(
                "Runs the current startup project and waits for it to finish. "
                "Progress messages from the application are streamed during execution. "
                "On success, returns the full output plus the run outcome: exitCode (absent "
                "if the process crashed or the terminal launch failed) and succeeded (exit "
                "code 0). "
                "On build failure, returns isError=true with structured content in the same "
                "format as list_issues (issues array + summary). "
                "Returns an error if there is no startup project, no active build configuration, "
                "or the project cannot currently be run.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .outputSchema(runToolOutputSchema),
        makeRunCallback(Utils::Id(Constants::NORMAL_RUN_MODE), "Run finished"));

    ToolRegistry::registerTool(
        Tool{}
            .name("debug_project")
            .title("Start debugging")
            .description(
                "Starts a debug session for the current startup project and returns immediately "
                "once the session has launched. "
                "On build failure, returns isError=true with structured content in the same "
                "format as list_issues (issues array + summary). "
                "Returns an error if there is no startup project, no active build configuration, "
                "or the project cannot currently be run in debug mode.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .outputSchema(runToolOutputSchema),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const Utils::Id runMode = Utils::Id(Constants::DEBUG_RUN_MODE);
            const Utils::Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(runMode);
            if (!canRun) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(canRun.error())));
                return ResultOk;
            }

            struct State
            {
                bool finished = false;
                QJsonObject failureIssues;
            };
            auto state = std::make_shared<State>();

            using namespace std::chrono_literals;

            const Utils::Result<ToolInterface::TaskProgressNotify> task = toolInterface.startTask(
                500ms,
                [state](Schema::Task t) {
                    if (state->finished)
                        Mcp::letTaskDieIn(t, 1min);
                    const bool failed = !state->failureIssues.isEmpty();
                    return t.status(
                        !state->finished ? Schema::TaskStatus::working
                        : failed         ? Schema::TaskStatus::failed
                                         : Schema::TaskStatus::completed);
                },
                [state]() -> Utils::Result<CallToolResult> {
                    if (!state->failureIssues.isEmpty())
                        return CallToolResult{}.isError(true).structuredContent(
                            QJsonObject{{"issues", state->failureIssues}});
                    return CallToolResult{}.isError(false).structuredContent(
                        QJsonObject{{"output", QString("Debug session started")}});
                },
                []() {},
                Mcp::progressToken(params));

            if (!task) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(task.error())));
                return ResultOk;
            }

            const ToolInterface::TaskProgressNotify notify = *task;

            auto rcStartedConn = std::make_shared<QMetaObject::Connection>();

            *rcStartedConn = QObject::connect(
                ProjectExplorerPlugin::instance(),
                &ProjectExplorerPlugin::runControlStarted,
                ProjectExplorerPlugin::instance(),
                [state, notify, rcStartedConn, runMode](RunControl *rc) {
                    if (rc->runMode() != runMode)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    state->finished = true;
                    if (notify)
                        notify(Schema::TaskStatus::completed, "Debug session started", std::nullopt);
                });

            QObject::connect(
                BuildManager::instance(),
                &BuildManager::buildQueueFinished,
                BuildManager::instance(),
                [state, notify, rcStartedConn](bool success) {
                    if (success || state->finished)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    state->finished = true;
                    state->failureIssues = issuesManager.getCurrentIssues();
                    const int errorCount = state->failureIssues.value("summary")
                                               .toObject()
                                               .value("errorCount")
                                               .toInt();
                    const QString statusMsg
                        = errorCount > 0 ? QString("Build failed with %1 error(s)").arg(errorCount)
                                         : QString("Build failed");
                    if (notify)
                        notify(Schema::TaskStatus::failed, statusMsg, std::nullopt);
                },
                Qt::SingleShotConnection);

            ProjectExplorerPlugin::runStartupProject(runMode, false);
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("search_in_files")
            .title("Search for pattern in project files")
            .description(
                "Search for a text pattern in files matching a file pattern within a "
                "project (or all projects) and return all matches")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file_pattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "File pattern to filter which files to search (e.g., '*.cpp', "
                             "'*.h')"}})
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Optional: name of the project to search in (searches all projects if "
                             "not specified)"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "case_sensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("file_pattern")
                    .addRequired("pattern")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString filePattern = p.value("file_pattern").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("case_sensitive").toBool(false);
            const std::optional<QString> projectName = p.contains("project_name")
                                                           ? std::optional<QString>(
                                                                 p.value("project_name").toString())
                                                           : std::nullopt;
            searchInFiles(
                filePattern,
                projectName,
                pattern,
                isRegex,
                caseSensitive,
                BuildManager::instance(),
                callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("replace_in_files")
            .title("Replace pattern in project files")
            .description(
                "Replace all matches of a text pattern in files matching a file pattern "
                "within a project (or all projects) with replacement text")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file_pattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "File pattern to filter which files to modify (e.g., '*.cpp', "
                             "'*.h')"}})
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Optional: name of the project to search in (searches all projects if "
                             "not specified)"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "replacement",
                        QJsonObject{{"type", "string"}, {"description", "Replacement text"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "case_sensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("file_pattern")
                    .addRequired("pattern")
                    .addRequired("replacement"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("ok", QJsonObject{{"type", "boolean"}})
                    .addRequired("ok")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString filePattern = p.value("file_pattern").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("case_sensitive").toBool(false);
            const std::optional<QString> projectName = p.contains("project_name")
                                                           ? std::optional<QString>(
                                                                 p.value("project_name").toString())
                                                           : std::nullopt;
            replaceInFiles(
                filePattern,
                projectName,
                pattern,
                replacement,
                isRegex,
                caseSensitive,
                BuildManager::instance(),
                callback);
        }));

    // ===== Original PE tools =====

    ToolRegistry::registerTool(
        Tool{}
            .name("get_build_status")
            .title("Get current build status")
            .description("Get current build progress and status")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("status", QJsonObject{{"type", "string"}})
                    .addRequired("status")),
        wrap([](const QJsonObject &) { return QJsonObject{{"status", getBuildStatus()}}; }));

    ToolRegistry::registerTool(
        Tool{}
            .name("find_files_in_projects")
            .title("Find files in project")
            .description("Find all files matching the pattern in a given project")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Name of the project to limit the search to (optional)"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Pattern for finding the file, either a glob pattern or a regex"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regex (default is false)"}})
                    .addRequired("pattern"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "files",
                        QJsonObject{
                            {"type", "array"},
                            {"description", "List of file paths matching the pattern"},
                            {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("files")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<Schema::CallToolResult> {
            const QJsonObject &p = params.argumentsAsObject();
            const QString projectName = p.value("project_name").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool();

            QRegularExpression re(
                isRegex ? pattern : QRegularExpression::wildcardToRegularExpression(pattern));
            if (!re.isValid()) {
                return CallToolResult{}.isError(true).structuredContent(
                    QJsonObject{{"error", "Invalid regex pattern"}});
            }

            const QList<Project *> projects = projectName.isEmpty() ? ProjectManager::projects()
                                                                    : projectsForName(projectName);
            const QStringList files = findFiles(projects, re);
            return CallToolResult{}
                .structuredContent(QJsonObject{{"files", QJsonArray::fromStringList(files)}})
                .isError(false);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("list_projects")
            .title("List all available projects")
            .description(
                "List all loaded projects. Each entry includes the project name, its file "
                "path, the active version control branch, and whether it is the current startup "
                "project (is_active). Use path or branch to disambiguate when multiple "
                "projects share the same display name (common in multi-worktree setups).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "projects",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"path", QJsonObject{{"type", "string"}}},
                                      {"branch", QJsonObject{{"type", "string"}}},
                                      {"type", QJsonObject{{"type", "string"}}},
                                      {"is_active", QJsonObject{{"type", "boolean"}}},
                                  }}}}})
                    .addRequired("projects")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"projects", listProjects()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_kits")
            .title("List all available kits")
            .description(
                "List all kits configured in Qt Creator. Each entry includes the kit name, "
                "its id, whether it is valid, whether it has warnings, whether it is the "
                "default kit, whether it was auto-detected (and SDK-provided), a "
                "filesystem-friendly name, and an issues array with validation messages for "
                "invalid or warning kits.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "kits",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"id", QJsonObject{{"type", "string"}}},
                                      {"valid", QJsonObject{{"type", "boolean"}}},
                                      {"has_warning", QJsonObject{{"type", "boolean"}}},
                                      {"is_default", QJsonObject{{"type", "boolean"}}},
                                      {"auto_detected", QJsonObject{{"type", "boolean"}}},
                                      {"sdk_provided", QJsonObject{{"type", "boolean"}}},
                                      {"file_system_friendly_name",
                                       QJsonObject{{"type", "string"}}},
                                      {"issues",
                                       QJsonObject{
                                           {"type", "array"},
                                           {"items", QJsonObject{{"type", "string"}}}}},
                                  }}}}})
                    .addRequired("kits")),
        wrap([](const QJsonObject &) { return QJsonObject{{"kits", listKits()}}; }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_project_kits")
            .title("List kits a project is configured for")
            .description(
                "List the kits a project is configured for (one per build target). "
                "Defaults to the active startup project when neither project_name nor "
                "project_path is given. Each kit entry has the same fields as list_kits "
                "plus is_active, which marks the kit of the project's active target. When "
                "multiple loaded projects share the same display name, pass project_path to "
                "disambiguate (returns reason:\"ambiguous_name\" with candidates otherwise).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name of the project. Optional; defaults to the active "
                             "startup project."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file. Unambiguously identifies "
                             "the project and takes precedence over project_name."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "object"}})
                    .addProperty("kits", QJsonObject{{"type", "array"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            return projectKits(
                p.value("project_name").toString(), p.value("project_path").toString());
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("add_kits_to_project")
            .title("Add kits to a project")
            .description(
                "Adds a build target for each of the given kits to a project. Kits may be "
                "identified by kit id or display name (see list_kits). Defaults to the "
                "active startup project when neither project_name nor project_path is given. "
                "Returns a per-kit results array with status added/already_present/"
                "not_found/failed; the call does not abort on the first error. When "
                "multiple loaded projects share the same display name, pass project_path to "
                "disambiguate.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "kits",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description",
                             "Kit ids or display names to add to the project."}})
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name of the project. Optional; defaults to the active "
                             "startup project."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file. Unambiguously identifies "
                             "the project and takes precedence over project_name."}})
                    .addRequired("kits"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "object"}})
                    .addProperty("results", QJsonObject{{"type", "array"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            QStringList kits;
            for (const QJsonValue &v : p.value("kits").toArray())
                kits.append(v.toString());
            return addKitsToProject(
                p.value("project_name").toString(), p.value("project_path").toString(), kits);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_build_configs")
            .title("List available build configurations")
            .description("List available build configurations")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "build_configs",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("build_configs")),
        wrap([](const QJsonObject &) {
            const QStringList configs = listBuildConfigs();
            QJsonArray arr;
            for (const QString &c : configs)
                arr.append(c);
            return QJsonObject{{"build_configs", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("switch_build_config")
            .title("Switch to a specific build configuration")
            .description("Switch to a specific build configuration")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the build configuration to switch to"}})
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString name = p.value("name").toString();
            return QJsonObject{{"success", switchToBuildConfig(name)}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_current_project")
            .title("Get the currently active project")
            .description("Get the currently active project")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Display name of the currently active project"},
                            })
                    .addProperty(
                        "project_file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Path to the project's main file (e.g., .pro, .vcxproj, "
                             "CMakeLists.txt)"},
                            })
                    .addProperty(
                        "project_directory",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to the project's directory"},
                            })
                    .addRequired("project_directory")),
        wrap([](const QJsonObject &) { return getCurrentProject(); }));

    ToolRegistry::registerTool(
        Tool{}
            .name("set_active_project")
            .title("Set the active startup project")
            .description(
                "Changes the active startup project (the one Qt Creator builds, runs, and "
                "debugs by default). Accepts project_name, project_path, or both. When "
                "multiple loaded projects share the same display name (e.g. the same "
                "project open in two Git worktrees), you must also supply project_path to "
                "disambiguate; the tool returns reason:\"ambiguous_name\" with a "
                "candidates array if project_path is omitted and the name matches more "
                "than one project.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name of the project to activate. Required unless "
                             "project_path alone is sufficient to identify it."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file. Unambiguously identifies "
                             "the project and takes precedence over project_name when "
                             "multiple projects share the same display name."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("active_project", QJsonObject{{"type", "object"}})
                    .addProperty("previous_active_project", QJsonObject{{"type", "object"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")
                    .addRequired("reason")
                    .addRequired("message")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const QString projectName = p.value("project_name").toString();
            const QString projectPath = p.value("project_path").toString();

            const ProjectResolution resolution
                = resolveTargetProject(projectName, projectPath, false);
            if (!resolution.project)
                return resolution.error;

            Project *previous = ProjectManager::startupProject();
            Project *target = resolution.project;

            if (previous == target) {
                return {
                    {"success", true},
                    {"reason", "ok_already_active"},
                    {"message",
                     QString("Project '%1' is already the active startup project.")
                         .arg(target->displayName())},
                    {"active_project", projectInfoObject(target)},
                    {"previous_active_project", projectInfoObject(previous)}};
            }

            ProjectManager::setStartupProject(target);

            return {
                {"success", true},
                {"reason", "ok"},
                {"message",
                 QString("Active startup project set to '%1'.").arg(target->displayName())},
                {"active_project", projectInfoObject(target)},
                {"previous_active_project",
                 previous ? QJsonValue(projectInfoObject(previous)) : QJsonValue()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("open_project")
            .title("Open a project")
            .description(
                "Opens a project in Qt Creator from a project file path (e.g., "
                "CMakeLists.txt, a .pro, .qbs, or .qmlproject file). If the project is "
                "already open, returns success with already_open=true. The opened project "
                "is added to the session; use set_active_project to make it the startup "
                "project.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description",
                             "Absolute path to the project file to open (e.g., "
                             "CMakeLists.txt, .pro, .qbs, .qmlproject)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("already_open", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "object"}})
                    .addRequired("success")
                    .addRequired("message")),
        wrap([](const QJsonObject &p) {
            return openProjectFile(p.value("path").toString());
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_current_build_config")
            .title("Get the currently active build configuration")
            .description("Get the currently active build configuration")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("build_config", QJsonObject{{"type", "string"}})
                    .addRequired("build_config")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"build_config", getCurrentBuildConfig()}};
        }));

    ToolRegistry::registerTool(
        Tool()
            .name("known_repositories_in_projects")
            .title("Get known version control repositories in all projects")
            .description(
                "List all known version control repositories (e.g., Git, Subversion) that are "
                "within the directories of all open projects")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema()
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the project to query repositories for"}})
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema()
                    .addProperty(
                        "repositories",
                        QJsonObject{
                            {"type", "object"},
                            {"description",
                             "Map of version control system names to lists of repository paths"}})
                    .addRequired("repositories")),
        wrap([](const QJsonObject &p) {
            const QString projectName = p.value("name").toString();
            const QMap<QString, QSet<QString>> repos = knownRepositoriesInProject(projectName);
            QJsonObject reposJson;
            for (auto it = repos.constBegin(); it != repos.constEnd(); ++it)
                reposJson[it.key()] = QJsonArray::fromStringList(Utils::toList(it.value()));
            return QJsonObject{{"repositories", reposJson}};
        }));

    ToolRegistry::registerTool(
        Tool()
            .name("project_dependencies")
            .title("List project dependencies for all projects")
            .description("List project dependencies for all projects")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the project to query dependencies for"}})
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "dependencies",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("dependencies")),
        wrap([](const QJsonObject &p) {
            const Utils::Result<QStringList> projects = projectDependencies(p["name"].toString());
            QJsonArray arr;
            for (const QString &pr : projects.value_or(QStringList{}))
                arr.append(pr);
            return QJsonObject{{"dependencies", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_run_configurations")
            .title("Get run configurations")
            .description(
                "Returns the project's existing run configurations. "
                "The result includes configuration names and which one is active.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "configurations",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"active", QJsonObject{{"type", "boolean"}}}}},
                                 {"required", QJsonArray{"name", "active"}}}},
                            {"description", "List of run configurations"}})
                    .addRequired("configurations")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"configurations", getRunConfigurations()}};
        }));

    // --- Device management tools -------------------------------------------

    ToolRegistry::registerTool(
        Tool{}
            .name("list_devices")
            .title("List configured devices")
            .description(
                "Lists all devices known to Qt Creator (ProjectExplorer::DeviceManager), with "
                "their id, type, display name, connection state, and SSH parameters.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(Tool::InputSchema{})
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("devices", QJsonObject{{"type", "array"}})
                    .addRequired("devices")),
        wrap([](const QJsonObject &) {
            QJsonArray devices;
            DeviceManager::forEachDevice([&devices](const IDeviceConstPtr &device) {
                devices.append(deviceToJson(device));
            });
            return QJsonObject{{"devices", devices}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("add_device")
            .title("Add a device")
            .description(
                "Creates a new device of the given device-type id (e.g. 'GenericLinuxOsType') and "
                "adds it to the DeviceManager, without going through the GUI wizard. 'params' sets "
                "the SSH parameters (host, port, userName, privateKeyFile, useKeyFile, timeout, "
                "hostKeyCheckingMode). Returns the new device id.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "type",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Device-type id, e.g. 'GenericLinuxOsType'. See the 'type' field of "
                             "list_devices, or use list_device_types."}})
                    .addProperty(
                        "displayName",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Display name for the new device (optional)."}})
                    .addProperty(
                        "params",
                        QJsonObject{
                            {"type", "object"},
                            {"description",
                             "SSH parameters: host, port, userName, privateKeyFile, useKeyFile, "
                             "timeout, hostKeyCheckingMode (none|strict|allowNoMatch)."}})
                    .addRequired("type"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("id", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const Id typeId = Id::fromString(p.value("type").toString());
            IDeviceFactory *factory = IDeviceFactory::find(typeId);
            if (!factory)
                return {{"success", false},
                        {"error", QString("Unknown device type: " + p.value("type").toString())}};
            if (!factory->canCreate())
                return {{"success", false}, {"error", "Device type cannot be created programmatically."}};

            IDevice::Ptr device = factory->construct();
            if (!device)
                return {{"success", false}, {"error", "Failed to construct device."}};

            if (!device->id().isValid())
                device->setupId(IDevice::ManuallyAdded);

            const QString displayName = p.value("displayName").toString();
            if (!displayName.isEmpty())
                device->setDisplayName(displayName);

            applySshParameters(device, p.value("params").toObject());

            DeviceManager::addDevice(device);
            qCInfo(mcpDevices) << "Added device" << device->id().toString()
                               << "of type" << typeId.toString();
            return {{"success", true}, {"id", device->id().toString()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("set_device_parameters")
            .title("Update device parameters")
            .description(
                "Updates the display name and/or SSH parameters of an existing device. Only the "
                "fields present in 'params' are changed; others keep their current values.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id",
                        QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addProperty(
                        "displayName",
                        QJsonObject{{"type", "string"}, {"description", "New display name (optional)."}})
                    .addProperty(
                        "params",
                        QJsonObject{
                            {"type", "object"},
                            {"description",
                             "SSH parameters to change: host, port, userName, privateKeyFile, "
                             "useKeyFile, timeout, hostKeyCheckingMode."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            IDevice::Ptr device = DeviceManager::find(Id::fromString(p.value("id").toString()));
            if (!device)
                return {{"success", false}, {"error", "No such device."}};

            const QString displayName = p.value("displayName").toString();
            if (!displayName.isEmpty())
                device->setDisplayName(displayName);

            if (p.contains("params"))
                applySshParameters(device, p.value("params").toObject());

            return {{"success", true}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("remove_device")
            .title("Remove a device")
            .description("Removes the device with the given id from the DeviceManager.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("id", QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const Id id = Id::fromString(p.value("id").toString());
            if (!DeviceManager::find(id))
                return {{"success", false}, {"error", "No such device."}};
            DeviceManager::removeDevice(id);
            return {{"success", true}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("test_device")
            .title("Test a device connection")
            .description(
                "Runs the device's connection tester (IDevice::createDeviceTester()), collecting "
                "the streamed progress and error messages, and returns the final result. Blocks "
                "until the test finishes or the timeout elapses.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("id", QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addProperty(
                        "timeoutSeconds",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Abort the test after this many seconds (default 60)."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "result",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"success", "failure", "timeout"}}})
                    .addProperty("messages", QJsonObject{{"type", "array"}})
                    .addRequired("result")
                    .addRequired("messages")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            IDevice::Ptr device = DeviceManager::find(Id::fromString(p.value("id").toString()));
            if (!device) {
                callback(
                    {{"result", "failure"},
                     {"messages",
                      QJsonArray{QJsonObject{{"kind", "error"}, {"text", "No such device."}}}}});
                return;
            }
            if (!device->hasDeviceTester()) {
                callback(
                    {{"result", "failure"},
                     {"messages",
                      QJsonArray{QJsonObject{
                          {"kind", "error"},
                          {"text", "Device type does not support testing."}}}}});
                return;
            }

            DeviceTester *tester = device->createDeviceTester();
            if (!tester) {
                callback(
                    {{"result", "failure"},
                     {"messages",
                      QJsonArray{QJsonObject{
                          {"kind", "error"}, {"text", "Could not create device tester."}}}}});
                return;
            }

            tester->setParent(Utils::shutdownGuard());
            auto messages = std::make_shared<QJsonArray>();
            auto finished = std::make_shared<bool>(false);

            const auto finish = [callback, tester, messages, finished](const QString &result) {
                if (*finished)
                    return;
                *finished = true;
                callback({{"result", result}, {"messages", *messages}});
                tester->deleteLater();
            };

            QObject::connect(
                tester, &DeviceTester::progressMessage, tester, [messages](const QString &msg) {
                    messages->append(QJsonObject{{"kind", "progress"}, {"text", msg}});
                });
            QObject::connect(
                tester, &DeviceTester::errorMessage, tester, [messages](const QString &msg) {
                    messages->append(QJsonObject{{"kind", "error"}, {"text", msg}});
                });
            QObject::connect(
                tester, &DeviceTester::finished, tester, [finish](DeviceTester::TestResult result) {
                    finish(result == DeviceTester::TestSuccess ? "success" : "failure");
                });

            const int timeoutSeconds = p.value("timeoutSeconds").toInt(60);
            QTimer::singleShot(timeoutSeconds * 1000, tester, [finish]() { finish("timeout"); });

            tester->testDevice();
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("detect_device_tools")
            .title("Detect tools on a device and create kits")
            .description(
                "Connects to the device and runs the same auto-detection as the device "
                "configuration's \"Run Auto-Detection Now\" button: detects toolchains and "
                "debuggers on the device, detects on-device build tools (rsync, cmake, ...), "
                "and then creates kits for the device. Use this to set up a remote "
                "build/run/debug environment without the GUI. Returns the kits now "
                "associated with the device.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id", QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("kits", QJsonObject{{"type", "array"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            IDevice::Ptr device = DeviceManager::find(Id::fromString(p.value("id").toString()));
            if (!device) {
                callback({{"success", false}, {"error", "No such device."}});
                return;
            }

            const auto reportKits = [device, callback] {
                // requestToolDetection() only creates kits when kit creation is enabled for
                // the device; create them explicitly so this tool always produces kits.
                // Skip if the device already has kits (from a prior run or the enabled
                // auto-creation) to avoid duplicates.
                const bool hasKits = Utils::anyOf(KitManager::kits(), [&](Kit *k) {
                    return BuildDeviceKitAspect::deviceId(k) == device->id();
                });
                if (!hasKits) {
                    KitManager::createKitsForBuildDevice(device);
                } else {
                    // Existing kits may predate the detection of some tools: a remote CMake
                    // tool, for example, only becomes detectable once the device is reachable,
                    // which is typically after the kits were first created. Re-complete the
                    // device's kits so newly detected tools get bound into aspects that are
                    // still unset (completeKit() runs setup() for those and fix() otherwise).
                    for (Kit *k : KitManager::kits()) {
                        if (BuildDeviceKitAspect::deviceId(k) == device->id())
                            KitManager::completeKit(k);
                    }
                }

                QJsonArray kits;
                for (Kit *k : KitManager::kits()) {
                    if (BuildDeviceKitAspect::deviceId(k) == device->id()
                        || RunDeviceKitAspect::deviceId(k) == device->id()) {
                        kits.append(QJsonObject{{"id", k->id().toString()},
                                                {"name", k->displayName()},
                                                {"valid", k->isValid()}});
                    }
                }
                callback({{"success", true}, {"kits", kits}});
            };

            const auto onConnected = [device, reportKits, callback](const Utils::Result<> &res) {
                if (!res) {
                    callback({{"success", false}, {"error", res.error()}});
                    return;
                }
                // Toolchain and debugger detection run synchronously here.
                device->requestToolDetection(device->toolSearchPaths());
                // On-device build tools (rsync, cmake, ...) are detected asynchronously;
                // create the kits once that has finished.
                GlobalTaskTree::start(device->autoDetectDeviceToolsRecipe(), {}, reportKits);
            };

            device->tryToConnect({Utils::shutdownGuard(), onConnected});
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_device_types")
            .title("List available device types")
            .description(
                "Lists the device-type ids that can be passed to add_device, with their display "
                "names and whether they can be created programmatically.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(Tool::InputSchema{})
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("types", QJsonObject{{"type", "array"}})
                    .addRequired("types")),
        wrap([](const QJsonObject &) {
            QJsonArray types;
            for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories()) {
                types.append(QJsonObject{
                    {"type", factory->deviceType().toString()},
                    {"displayName", factory->displayName()},
                    {"canCreate", factory->canCreate()},
                });
            }
            return QJsonObject{{"types", types}};
        }));
}

} // namespace ProjectExplorer::Internal
