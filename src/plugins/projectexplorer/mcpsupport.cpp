// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildconfiguration.h"
#include "buildmanager.h"
#include "editorconfiguration.h"
#include "issuesmanager.h"
#include "kit.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "runconfiguration.h"
#include "runcontrol.h"
#include "target.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/vcsmanager.h>

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filesearch.h>
#include <utils/result.h>

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>

using namespace Utils;

namespace ProjectExplorer::Internal {

static QList<Project *> projectsForName(const QString &name)
{
    return Utils::filtered(ProjectManager::projects(), Utils::equal(&Project::displayName, name));
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

static QStringList listProjects()
{
    QStringList projects;
    for (Project *project : ProjectManager::projects())
        projects.append(project->displayName());
    return projects;
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
        {"projectName", project->displayName()},
        {"projectFile", project->projectFilePath().toUserOutput()},
        {"projectDirectory", project->projectDirectory().toUserOutput()},
    };
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

    // Output schema for `build`. Designed so the AI doesn't have to inspect
    // `issues` to guess the build verdict — `succeeded` is the single source
    // of truth, mirrored by the tool's TaskStatus (completed vs failed).
    const auto buildOutputSchema =
        Tool::OutputSchema{}
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
            .addRequired("succeeded")
            .addRequired("error_count")
            .addRequired("warning_count")
            .addRequired("duration_ms")
            .addRequired("issues")
            .addRequired("summary_text");

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
                "the issues array, and don't call get_build_status afterwards (that tool "
                "reports current activity, not the verdict of a finished build).")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .inputSchema(
                Tool::InputSchema().addProperty(
                    "projectName",
                    QJsonObject{
                        {"description",
                         "Name of the project to build. Defaults to the active startup "
                         "project."},
                        {"type", "string"}}))
            .outputSchema(buildOutputSchema)
            .annotations(
                Schema::ToolAnnotations()
                    .destructiveHint(false)
                    .idempotentHint(true)
                    .openWorldHint(false)
                    .readOnlyHint(false)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QString projectName = params.arguments()->value("projectName").toString();

            QList<Project *> projects{ProjectManager::startupProject()};
            if (!projectName.isEmpty())
                projects = projectsForName(projectName);

            projects.removeIf([](Project *p) { return !p; });

            if (projects.isEmpty())
                return ResultError("No project named '" + projectName + "' found");

            // Shared state between the buildQueueFinished signal handler and
            // the heartbeat. Both are needed: the signal carries the
            // success/failure boolean (BuildManager::currentProgress can only
            // tell us "still running" vs "not running", never "the most
            // recent build failed"); the heartbeat is what transitions the
            // task to `completed` or `failed`. The `timer` is used so the
            // duration we report is wall-clock from the buildProjects() call
            // — including dependency resolution and pre-build steps — rather
            // than just the compile phase.
            struct State {
                bool finished = false;
                bool succeeded = true;
                QElapsedTimer timer;
            };
            auto state = std::make_shared<State>();
            state->timer.start();

            // Connect BEFORE buildProjects() to avoid losing a synchronously
            // emitted buildQueueFinished — e.g. when everything is already
            // up-to-date and the queue drains in one tick. SingleShotConnection
            // ensures we capture exactly one verdict per tool invocation.
            QObject::connect(
                BuildManager::instance(),
                &BuildManager::buildQueueFinished,
                BuildManager::instance(),
                [state](bool success) {
                    state->succeeded = success;
                    state->finished = true;
                },
                Qt::SingleShotConnection);

            BuildManager::buildProjects(projects, ConfigSelection::Active);

            using namespace std::chrono_literals;

            toolInterface.startTask(
                1s,
                [state](Schema::Task task) -> Schema::Task {
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
                [state]() -> Utils::Result<Schema::CallToolResult> {
                    const QJsonObject issuesData = issuesManager.getCurrentIssues();
                    const QJsonObject issuesSummary = issuesData.value("summary").toObject();
                    const int errorCount = issuesSummary.value("errorCount").toInt();
                    const int warningCount = issuesSummary.value("warningCount").toInt();
                    const qint64 durationMs = state->timer.elapsed();

                    QString summaryText;
                    if (state->succeeded) {
                        summaryText
                            = warningCount == 0
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
                        .structuredContent(QJsonObject{
                            {"succeeded", state->succeeded},
                            {"error_count", errorCount},
                            {"warning_count", warningCount},
                            {"duration_ms", durationMs},
                            {"issues", issuesData.value("issues")},
                            {"summary_text", summaryText},
                        })
                        .isError(!state->succeeded);
                },
                []() { BuildManager::cancel(); },
                Mcp::progressToken(params));

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
                    return CallToolResult{}.isError(false).structuredContent(
                        QJsonObject{{"output", state->output.join('\n')}});
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
                [state, notify, rcStartedConn, runMode, finishedMessage](RunControl *rc) {
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
                    QObject::connect(
                        rc,
                        &RunControl::stopped,
                        rc,
                        [state, notify, finishedMessage]() {
                            state->finished = true;
                            if (notify)
                                notify(Schema::TaskStatus::completed, finishedMessage, std::nullopt);
                        },
                        Qt::SingleShotConnection);
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
                "On success, returns the full output. "
                "On build failure, returns isError=true with structured content in the same "
                "format as list_issues (issues array + summary). "
                "Returns an error if there is no startup project, no active build configuration, "
                "or the project cannot currently be run.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .outputSchema(runToolOutputSchema),
        makeRunCallback(Utils::Id(Constants::NORMAL_RUN_MODE), "Run finished"));

    ToolRegistry::registerTool(
        Tool{}
            .name("debug")
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
                        "filePattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "File pattern to filter which files to search (e.g., '*.cpp', "
                             "'*.h')"}})
                    .addProperty(
                        "projectName",
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
                        "caseSensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("filePattern")
                    .addRequired("pattern")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString filePattern = p.value("filePattern").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            const std::optional<QString> projectName = p.contains("projectName")
                                                           ? std::optional<QString>(
                                                                 p.value("projectName").toString())
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
                        "filePattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "File pattern to filter which files to modify (e.g., '*.cpp', "
                             "'*.h')"}})
                    .addProperty(
                        "projectName",
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
                        "caseSensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("filePattern")
                    .addRequired("pattern")
                    .addRequired("replacement"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("ok", QJsonObject{{"type", "boolean"}})
                    .addRequired("ok")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString filePattern = p.value("filePattern").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            const std::optional<QString> projectName = p.contains("projectName")
                                                           ? std::optional<QString>(
                                                                 p.value("projectName").toString())
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
                        "projectName",
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
            const QString projectName = p.value("projectName").toString();
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
            .description("List all available projects")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "projects",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("projects")),
        wrap([](const QJsonObject &) {
            const QStringList projects = listProjects();
            QJsonArray arr;
            for (const QString &pr : projects)
                arr.append(pr);
            return QJsonObject{{"projects", arr}};
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
                        "buildConfigs",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("buildConfigs")),
        wrap([](const QJsonObject &) {
            const QStringList configs = listBuildConfigs();
            QJsonArray arr;
            for (const QString &c : configs)
                arr.append(c);
            return QJsonObject{{"buildConfigs", arr}};
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
                        "projectName",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Display name of the currently active project"},
                            })
                    .addProperty(
                        "projectFile",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Path to the project's main file (e.g., .pro, .vcxproj, "
                             "CMakeLists.txt)"},
                            })
                    .addProperty(
                        "projectDirectory",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to the project's directory"},
                            })
                    .addRequired("projectDirectory")),
        wrap([](const QJsonObject &) { return getCurrentProject(); }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_current_build_config")
            .title("Get the currently active build configuration")
            .description("Get the currently active build configuration")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("buildConfig", QJsonObject{{"type", "string"}})
                    .addRequired("buildConfig")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"buildConfig", getCurrentBuildConfig()}};
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
}

} // namespace ProjectExplorer::Internal
