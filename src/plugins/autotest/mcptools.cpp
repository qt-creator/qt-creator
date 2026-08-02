// Copyright (C) 2026 Jeff Heller
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcptools.h"

#include "autotestconstants.h"
#include "testconfiguration.h"
#include "testcodeparser.h"
#include "testresultsmanager.h"
#include "testrunner.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <projectexplorer/issuesmanager.h>

#include <utils/qtcassert.h>
#include <utils/result.h>

#include <chrono>

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTimer>

using namespace Mcp;
using namespace Utils;

namespace Schema = Mcp::Schema;

static Q_LOGGING_CATEGORY(mcpAutotest, "qtc.autotest.mcptools", QtWarningMsg)

namespace Autotest::Internal {

// ----------------------------------------------------------------------------
// Test command helpers
// ----------------------------------------------------------------------------

static TestResultsManager &resultsManager()
{
    static TestResultsManager manager;
    return manager;
}

struct ResolvedTestRun
{
    TestRunMode mode;
    QList<ITestConfiguration *> configs;
};

// "_"-prefixed keys are reserved by the protocol and not ours to reject.
static Utils::Result<> rejectUnknownArgs(const QJsonObject &args, const QStringList &known)
{
    QStringList unknown;
    for (auto it = args.begin(), end = args.end(); it != end; ++it) {
        if (!it.key().startsWith('_') && !known.contains(it.key()))
            unknown.append(it.key());
    }
    if (unknown.isEmpty())
        return ResultOk;
    return ResultError(QString("Unknown argument(s): %1 (expected %2)")
                           .arg(unknown.join(", "), known.join('/')));
}

static QStringList knownScopes() { return {"all", "selected", "failed", "named"}; }
static QStringList knownModes() { return {"run", "debug"}; }

// How often to re-check whether test discovery can still deliver.
constexpr std::chrono::seconds parserPollInterval{30};

// Argument-only checks. Kept out of resolveTestRun so a bad call fails now
// instead of being deferred until the parser finishes, which on a CTest-only
// project may be never.
static Utils::Result<> validateRunArgs(
    const QString &scope, const QStringList &names, const QString &mode)
{
    if (!knownScopes().contains(scope)) {
        return ResultError(
            QString("Unknown scope: %1 (expected %2)").arg(scope, knownScopes().join('/')));
    }
    if (!knownModes().contains(mode))
        return ResultError(
            QString("Unknown mode: %1 (expected %2)").arg(mode, knownModes().join('/')));

    if (scope == QLatin1String("named")) {
        if (names.isEmpty())
            return ResultError("scope=named requires a non-empty names array");
    } else if (!names.isEmpty()) {
        return ResultError(
            QString("names is only used with scope=\"named\", but scope is \"%1\". "
                    "Pass scope=\"named\" to run just those tests.")
                .arg(scope));
    }
    return ResultOk;
}

struct RunArgs
{
    QString scope;
    QString mode;
    QStringList names;
};

// Types are checked here too: toString()/toArray() turn a wrong type into the
// default, and for scope that default is a silent full run.
static Utils::Result<RunArgs> readRunArgs(const QJsonObject &p)
{
    if (const Utils::Result<> known = rejectUnknownArgs(p, {"scope", "names", "mode"}); !known)
        return ResultError(known.error());

    const auto readString = [&p](const QString &key,
                                 const QString &fallback) -> Utils::Result<QString> {
        const QJsonValue v = p.value(key);
        if (v.isUndefined() || v.isNull())
            return fallback;
        if (!v.isString())
            return ResultError(QString("%1 must be a string").arg(key));
        return v.toString();
    };

    RunArgs args;
    const Utils::Result<QString> scope = readString("scope", "all");
    if (!scope)
        return ResultError(scope.error());
    args.scope = *scope;
    const Utils::Result<QString> mode = readString("mode", "run");
    if (!mode)
        return ResultError(mode.error());
    args.mode = *mode;

    const QJsonValue namesValue = p.value("names");
    if (!namesValue.isUndefined() && !namesValue.isNull() && !namesValue.isArray())
        return ResultError("names must be an array of strings");
    const QJsonArray namesArr = namesValue.toArray();
    args.names.reserve(namesArr.size());
    for (const QJsonValue &v : namesArr) {
        if (!v.isString())
            return ResultError("names must contain strings only");
        args.names.append(v.toString());
    }

    if (const Utils::Result<> ok = validateRunArgs(args.scope, args.names, args.mode); !ok)
        return ResultError(ok.error());
    return args;
}

static Utils::Result<ResolvedTestRun> resolveTestRun(
    const QString &scope, const QStringList &names, const QString &mode)
{
    qCDebug(mcpAutotest) << "resolveTestRun: scope=" << scope << " names=" << names
                         << " mode=" << mode;

    TestTreeModel *model = TestTreeModel::instance();
    if (!model)
        return ResultError("Autotest plugin not available");

    const TestRunMode runMode = mode == QLatin1String("debug") ? TestRunMode::Debug
                                                               : TestRunMode::Run;

    QList<ITestConfiguration *> configs;
    if (scope == QLatin1String("selected")) {
        configs = model->getSelectedTests(runMode);
    } else if (scope == QLatin1String("failed")) {
        configs = model->getFailedTests();
    } else if (scope == QLatin1String("all")) {
        configs = model->getAllTestCases(runMode);
    } else if (scope == QLatin1String("named")) {
        auto lookupByName = [model](const QString &qualifiedName) -> QList<ITestTreeItem *> {
            // A test name may contain "::" itself, so the whole name wins over
            // reading it as Class::function.
            const QList<ITestTreeItem *> whole = model->testItemsByName(qualifiedName);
            if (!whole.isEmpty())
                return whole;
            const int sep = qualifiedName.indexOf(QStringLiteral("::"));
            if (sep < 0)
                return {};
            const QString className = qualifiedName.left(sep);
            const QString functionName = qualifiedName.mid(sep + 2);
            QList<ITestTreeItem *> matched;
            for (ITestTreeItem *classItem : model->testItemsByName(className)) {
                for (int i = 0, n = classItem->childCount(); i < n; ++i) {
                    ITestTreeItem *child = classItem->childAt(i);
                    if (child && child->name() == functionName)
                        matched.append(child);
                }
            }
            return matched;
        };

        QList<QList<ITestTreeItem *>> itemsPerName;
        itemsPerName.reserve(names.size());
        QStringList notFound;
        for (const QString &name : names) {
            const QList<ITestTreeItem *> items = lookupByName(name);
            if (items.isEmpty())
                notFound.append(name);
            else
                itemsPerName.append(items);
        }
        if (!notFound.isEmpty())
            return ResultError(
                QString("Test name(s) not found in the current Autotest model: %1. "
                        "If you recently added or renamed these tests, call reconfigure "
                        "first to trigger a re-parse, then retry.")
                    .arg(notFound.join(", ")));
        // Let each root turn its own items into configurations: the default is
        // one per item, CTest merges them into a single ctest invocation.
        // roots keeps the encounter order - iterating the hash would leave the
        // order the tests run in undefined.
        QList<ITestTreeItem *> roots;
        QHash<ITestTreeItem *, QList<ITestTreeItem *>> itemsPerRoot;
        for (const QList<ITestTreeItem *> &items : itemsPerName) {
            for (ITestTreeItem *item : items) {
                ITestTreeItem *root = item;
                while (root && root->type() != ITestTreeItem::Root)
                    root = static_cast<ITestTreeItem *>(root->parent());
                QTC_ASSERT(root, continue);
                if (!itemsPerRoot.contains(root))
                    roots.append(root);
                itemsPerRoot[root].append(item);
            }
        }
        for (ITestTreeItem *root : roots)
            configs.append(root->getTestConfigurationsForItems(itemsPerRoot.value(root), runMode));
    }

    if (configs.isEmpty())
        return ResultError(QString("No tests to run for scope: %1").arg(scope));

    return ResolvedTestRun{runMode, configs};
}

static void cancelTestRun()
{
    if (!resultsManager().isRunning())
        return;
    TestRunner *runner = TestRunner::instance();
    if (!runner)
        return;
    emit runner->requestStopTestRun();
}

static bool discoveryCanStillArrive(TestTreeModel *model)
{
    return model && model->parser()->isParsingOrScheduled();
}

static QJsonObject testRunStatus()
{
    QJsonObject out;
    out.insert("running", resultsManager().isRunning());
    const QJsonObject summary = resultsManager().summary();
    const QJsonObject summaryCounts = summary.value("summary").toObject();
    const int total = summaryCounts.value("total").toInt();
    const bool buildFailed = summaryCounts.value("build_failed").toBool();
    out.insert("has_snapshot", total > 0 || buildFailed);
    out.insert("summary_text", summary.value("summary_text"));
    return out;
}

// ----------------------------------------------------------------------------
// MCP Tool registrations
// ----------------------------------------------------------------------------

void registerMcpTools()
{
    using Tool = Schema::Tool;
    using ToolAnnotations = Schema::ToolAnnotations;
    using CallToolResult = Schema::CallToolResult;
    using ToolExecution = Schema::ToolExecution;

    // Constructed here, not where it is read: it only ever learns about a task
    // from a TaskHub signal, so it has to be connected before the first build.
    // Leaked on purpose: its destructor runs at exit(), on released memory.
    static ProjectExplorer::IssuesManager &issuesManager = *new ProjectExplorer::IssuesManager;

    using SimplifiedCallback = std::function<Utils::Result<QJsonObject>(const QJsonObject &)>;
    const auto wrap = [](SimplifiedCallback &&callback) -> Server::ToolCallback {
        return [callback = std::move(callback)](
                   const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const Utils::Result<QJsonObject> result = callback(params.argumentsAsObject());
            if (!result)
                return ResultError(result.error());
            return CallToolResult{}.isError(false).structuredContent(*result);
        };
    };

    // For tools that declare an input schema. Without one the tool forbids
    // nothing, so rejecting arguments there would refuse calls it allows.
    const auto wrapChecked = [](const QStringList &knownArgs,
                                SimplifiedCallback &&callback) -> Server::ToolCallback {
        return [knownArgs, callback = std::move(callback)](
                   const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            if (const Utils::Result<> known = rejectUnknownArgs(args, knownArgs); !known)
                return ResultError(known.error());
            const Utils::Result<QJsonObject> result = callback(args);
            if (!result)
                return ResultError(result.error());
            return CallToolResult{}.isError(false).structuredContent(*result);
        };
    };

    const auto testSummaryOutputSchema
        = Tool::OutputSchema{}
              .addProperty(
                  "summary",
                  QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                           {"passed", QJsonObject{{"type", "integer"}}},
                           {"failed", QJsonObject{{"type", "integer"}}},
                           {"skipped", QJsonObject{{"type", "integer"}}},
                           {"fatal",
                            QJsonObject{
                                {"type", "integer"}, {"description", "Tests aborted by qFatal()"}}},
                           {"blacklisted",
                            QJsonObject{
                                {"type", "integer"},
                                {"description",
                                 "Tests with a Blacklisted{Pass,Fail,XPass,XFail} outcome. "
                                 "Qt Test treats these as 'result does not matter for overall "
                                 "result' — the project has explicitly opted out. Counted "
                                 "here for completeness but NOT included in passed/failed and "
                                 "NOT listed in the failures[] array."}}},
                           {"total", QJsonObject{{"type", "integer"}}},
                           {"duration_ms",
                            QJsonObject{
                                {"type", "integer"},
                                {"minimum", -1},
                                {"description",
                                 "Whole-run duration in milliseconds. -1 if autotest "
                                 "didn't report a duration."}}},
                           {"build_failed", QJsonObject{{"type", "boolean"}}}}},
                      {"required",
                       QJsonArray{
                           "passed",
                           "failed",
                           "skipped",
                           "fatal",
                           "blacklisted",
                           "total",
                           "duration_ms",
                           "build_failed"}}})
              .addProperty(
                  "failures",
                  QJsonObject{
                      {"type", "array"},
                      {"items", QJsonObject{{"type", "string"}}},
                      {"description",
                       "Names of tests that failed, fataled, or were skipped. Use "
                       "get_test_details with these names for full info."}})
              .addProperty(
                  "tests_with_warnings",
                  QJsonObject{
                      {"type", "array"},
                      {"items", QJsonObject{{"type", "string"}}},
                      {"description",
                       "Names of passing tests that emitted any qWarning/qCritical/"
                       "qFatal/qSystemMsg in their function. Useful for projects that "
                       "forbid warnings on passing tests."}})
              .addProperty("summary_text", QJsonObject{{"type", "string"}})
              .addProperty(
                  "build_issues",
                  QJsonObject{
                      {"type", "array"},
                      {"description",
                       "Build errors/warnings from the pre-test build. Present only when "
                       "summary.build_failed is true — folded in by run_tests so the AI "
                       "can diagnose the build failure without a separate list_issues "
                       "call. Same shape as list_issues' issues array (objects with type, "
                       "description, file, line, id). Absent when the build succeeded."}})
              .addRequired("summary")
              .addRequired("failures")
              .addRequired("tests_with_warnings")
              .addRequired("summary_text");

    // ---- run_tests (long-running async tool) ----
    ToolRegistry::registerTool(
        Tool{}
            .name("run_tests")
            .title("Run tests")
            .description(
                "Build (if needed) and run autotests, then return a compact summary: "
                "counts, list of failed/fatal/skipped test names, and list of passing "
                "test names that emitted warnings. Runs the whole suite unless you pass "
                "scope='named' with names - prefer that when you already know which "
                "tests you care about; a full run can be slow enough to hit a client "
                "timeout. Equivalent to clicking Run (or Debug, with mode='debug') in "
                "the Tests pane. Returns once the run finishes. "
                "To see per-test details (messages, file/line, etc.), follow up with "
                "get_test_details using ANY test name from the run — failures, "
                "warnings, or just a passing test you want to inspect. get_test_details "
                "returns the full message log for every named test regardless of its "
                "outcome; an empty messages[] means the test simply didn't emit anything. "
                "NOTE: each call replaces the current snapshot — if the user has just "
                "run something interesting in the UI, call get_last_test_results first "
                "instead of clobbering it.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "scope",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"all", "selected", "failed", "named"}},
                            {"default", "all"},
                            {"description",
                             "Which tests to run. 'all' runs every discovered test "
                             "(default). 'selected' means whatever the user has ticked in "
                             "the Tests pane, not a selection the caller passes, so it is "
                             "rarely what a caller wants. 'failed' re-runs the tests that "
                             "failed in the previous run. 'named' runs only the tests in "
                             "the `names` array; set this to run one test, as `names` with "
                             "any other scope is rejected."}})
                    .addProperty(
                        "names",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description",
                             "Test names to run. Requires scope='named'; passing names "
                             "with any other scope is an error. Use 'Class', or "
                             "'Class::function' for frameworks that list functions - "
                             "CTest entries have none, so only the whole test runs. "
                             "Names typically come from `failures` or `tests_with_warnings` "
                             "in a previous summary, or from list_tests. Names must already "
                             "be present in Autotest's current model — if a name is not "
                             "found, call reconfigure first to trigger a re-parse (needed "
                             "after adding or renaming test functions)."}})
                    .addProperty(
                        "mode",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"run", "debug"}},
                            {"default", "run"},
                            {"description",
                             "How to execute. 'run' is the normal mode. 'debug' runs them "
                             "under the debugger, which can reproduce timing-sensitive "
                             "failures that don't manifest in plain Run mode (e.g. "
                             "qFatals that only fire when stepped through). Debug mode is "
                             "much slower; use it for narrowing in on a known failure."}}))
            .outputSchema(testSummaryOutputSchema)
            .annotations(ToolAnnotations{}.readOnlyHint(false)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject p = params.argumentsAsObject();

            // Before startTask(): a call rejected here never runs, and a task
            // left behind would be reported completed by its own heartbeat.
            const Utils::Result<RunArgs> args = readRunArgs(p);
            if (!args) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(args.error())));
                return ResultOk;
            }
            const QString scope = args->scope;
            const QString mode = args->mode;
            const QStringList names = args->names;

            struct State
            {
                bool cancelRequested = false;
                bool waitingForParser = false;
                std::optional<QString> resolveError;
                std::optional<Schema::TaskStatus> finalStatus;
                ToolInterface::TaskProgressNotify notify;

                bool finished() const { return finalStatus.has_value(); }

                // The only way to end the task, so the status the client is
                // notified with is the one the heartbeat reports afterwards.
                // A cancelled run still ends through runFinished, which cannot
                // tell that apart from a normal end, so honour the request here.
                void finish(Schema::TaskStatus status, const QString &message)
                {
                    const bool cancelled = cancelRequested
                                           && status == Schema::TaskStatus::completed;
                    finalStatus = cancelled ? Schema::TaskStatus::cancelled : status;
                    if (notify)
                        notify(*finalStatus, cancelled ? "Cancelled" : message, std::nullopt);
                }
            };
            auto state = std::make_shared<State>();

            // startTask is always called synchronously so the client gets a task
            // ID immediately.  The actual resolve+run is deferred via startRun
            // if the parser is still scanning (see below).
            using namespace std::chrono_literals;
            const Utils::Result<ToolInterface::TaskProgressNotify> task = toolInterface.startTask(
                500ms,
                [state](Schema::Task t) -> Schema::Task {
                    if (state->finished()) {
                        letTaskDieIn(t, 1min);
                        // The heartbeat outlives the notify() that set the outcome,
                        // so it reports what finish() recorded rather than deriving it.
                        return t.status(*state->finalStatus);
                    }
                    const char *msg = state->cancelRequested  ? "Cancelling tests..."
                                    : state->waitingForParser ? "Waiting for test discovery..."
                                                              : "Running tests...";
                    t.statusMessage(QString::fromLatin1(msg));
                    return t.status(Schema::TaskStatus::working);
                },
                [state]() -> Utils::Result<CallToolResult> {
                    if (state->resolveError)
                        return CallToolResult{}.isError(true).addContent(
                            Schema::TextContent{}.text(*state->resolveError));
                    QJsonObject result = resultsManager().summary();
                    // Fold build issues inline when the build that gates the
                    // test run failed. Saves the AI a separate list_issues
                    // call to find out WHY the build broke.
                    if (result.value("summary").toObject().value("build_failed").toBool()) {
                        const QJsonObject issuesData = issuesManager.getBuildIssues();
                        result.insert("build_issues", issuesData.value("issues"));
                    }
                    return CallToolResult{}.isError(false).structuredContent(result);
                },
                [state]() {
                    state->cancelRequested = true;
                    cancelTestRun();
                },
                progressToken(params));

            if (!task) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(task.error())));
                return ResultOk;
            }

            state->notify = *task;

            // Launches an already-resolved run (resolution happens below).
            auto startRun = [state](ResolvedTestRun resolved) {
                state->waitingForParser = false;

                if (state->cancelRequested) {
                    qDeleteAll(resolved.configs); // owned here
                    state->finish(Schema::TaskStatus::cancelled, "Cancelled");
                    return;
                }

                auto conn = std::make_shared<QMetaObject::Connection>();
                *conn = QObject::connect(
                    &resultsManager(),
                    &TestResultsManager::runFinished,
                    &resultsManager(),
                    [state, conn]() {
                        QObject::disconnect(*conn);
                        if (state->finished())
                            return;
                        state->finish(Schema::TaskStatus::completed, "Tests finished");
                    });

                if (!resultsManager().runTests(resolved.mode, resolved.configs)) {
                    qDeleteAll(resolved.configs);
                    QObject::disconnect(*conn);
                    state->resolveError = "Failed to start test run (already in progress?)";
                    state->finish(Schema::TaskStatus::failed, "Failed to start");
                    return;
                }

                QTimer::singleShot(5000, &resultsManager(), [state]() {
                    if (state->finished())
                        return;
                    if (!resultsManager().isRunning())
                        return;
                    TestRunner *runner = TestRunner::instance();
                    if (!runner || !runner->isTestRunning())
                        resultsManager().recoverFromStuckRun();
                });
            };

            // Re-resolve against the settled tree, then run or report the error.
            auto resolveAndRun = [scope, names, mode, state, startRun]() {
                if (state->finished())
                    return;
                if (state->cancelRequested) {
                    state->finish(Schema::TaskStatus::cancelled, "Cancelled");
                    return;
                }
                Utils::Result<ResolvedTestRun> resolved = resolveTestRun(scope, names, mode);
                if (!resolved) {
                    state->resolveError = resolved.error();
                    state->finish(Schema::TaskStatus::failed, "Error");
                    return;
                }
                startRun(std::move(*resolved));
            };

            TestTreeModel *model = TestTreeModel::instance();

            // Wait for the parser to settle, then re-resolve and run. Queued so
            // it runs after TestTreeModel::sweep(); re-checks each time since a
            // build can trigger several scans.
            auto deferUntilParsed = [model, resolveAndRun, state]() {
                state->waitingForParser = true;
                qCDebug(mcpAutotest)
                    << "run_tests: deferring until the test parser finishes discovery";
                TestCodeParser *parser = model->parser();
                auto connFinished = std::make_shared<QMetaObject::Connection>();
                auto connFailed = std::make_shared<QMetaObject::Connection>();
                auto stopWaiting = [connFinished, connFailed, state] {
                    QObject::disconnect(*connFinished);
                    QObject::disconnect(*connFailed);
                    state->waitingForParser = false;
                };
                auto onParsingDone = [stopWaiting, resolveAndRun, state]() {
                    // A queued metacall posted before stopWaiting() still arrives.
                    if (state->finished() || !state->waitingForParser)
                        return;
                    if (state->cancelRequested) {
                        stopWaiting();
                        state->finish(Schema::TaskStatus::cancelled, "Cancelled");
                        return;
                    }
                    TestTreeModel *liveModel = TestTreeModel::instance();
                    if (discoveryCanStillArrive(liveModel)) {
                        qCDebug(mcpAutotest)
                            << "run_tests: parsingFinished fired but another scan is pending, "
                               "waiting for full convergence";
                        return; // connections stay installed
                    }
                    // Disconnect before resolveAndRun() to avoid double-dispatch.
                    stopWaiting();
                    resolveAndRun();
                };
                *connFinished = QObject::connect(
                    parser, &TestCodeParser::parsingFinished,
                    parser, onParsingDone, Qt::QueuedConnection);
                *connFailed = QObject::connect(
                    parser, &TestCodeParser::parsingFailed,
                    parser, onParsingDone, Qt::QueuedConnection);
                // A parser that stops without emitting - the project was closed
                // mid-scan, say - would otherwise leave the task waiting for good.
                // A scan still running is not that case, so keep waiting for it.
                auto *watchdog = new QTimer(parser);
                watchdog->setInterval(parserPollInterval);
                QObject::connect(
                    watchdog, &QTimer::timeout, parser,
                    [watchdog, stopWaiting, resolveAndRun, state] {
                        if (state->finished() || !state->waitingForParser) {
                            watchdog->deleteLater();
                            return;
                        }
                        if (discoveryCanStillArrive(TestTreeModel::instance()))
                            return;
                        watchdog->deleteLater();
                        qCDebug(mcpAutotest) << "run_tests: parser went quiet, resolving now";
                        stopWaiting();
                        resolveAndRun();
                    });
                watchdog->start();
            };

            // Resolve and run. A scan still to come can add the missing name, so
            // wait for it, but only while one can still happen.
            const bool discoveryPending = discoveryCanStillArrive(model);
            Utils::Result<ResolvedTestRun> resolved = resolveTestRun(scope, names, mode);
            if (resolved) {
                startRun(std::move(*resolved));
            } else if (discoveryPending) {
                deferUntilParsed();
            } else {
                // Not found, parser idle — final answer.
                state->resolveError = resolved.error();
                state->finish(Schema::TaskStatus::failed, "Error");
            }

            return ResultOk;
        });

    // ---- get_last_test_results (sync read-only) ----
    ToolRegistry::registerTool(
        Tool{}
            .name("get_last_test_results")
            .title("Read the most recent test run summary")
            .description(
                "Read-only summary of the most recent test run. Reflects whatever was "
                "last executed — by run_tests OR by the user clicking Run/Debug in the "
                "Tests pane. Returns counts plus name lists for failures and "
                "tests-with-warnings. Use get_test_details with specific test names to "
                "see per-test messages, file/line, and full debug log. Calling run_tests "
                "instead would re-execute and potentially erase a flaky or debug-only "
                "failure.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(testSummaryOutputSchema),
        wrap([](const QJsonObject &) { return resultsManager().summary(); }));

    // ---- get_test_status (sync read-only) ----
    ToolRegistry::registerTool(
        Tool{}
            .name("get_test_status")
            .title("Check test-run state without clobbering")
            .description(
                "Reports whether a test run is currently in progress and "
                "whether the snapshot holds results worth looking at. Call this before "
                "run_tests if there's any chance the user has just produced an "
                "interesting result (e.g. a debug-mode failure) in the Tests pane that "
                "you don't want to overwrite.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("running", QJsonObject{{"type", "boolean"}})
                    .addProperty("has_snapshot", QJsonObject{{"type", "boolean"}})
                    .addProperty("summary_text", QJsonObject{{"type", "string"}})
                    .addRequired("running")
                    .addRequired("has_snapshot")),
        wrap([](const QJsonObject &) { return testRunStatus(); }));

    // Per-test item schema. Declared explicitly so consumers know exactly
    // what fields each `tests[]` entry carries — and, critically, so the
    // `messages` field can carry its own absence-vs-filtering note (the
    // most common AI misconception is "empty messages means the tool
    // filtered them" — it doesn't).
    const QJsonObject perTestMessageItemSchema{
        {"type", "object"},
        {"properties",
         QJsonObject{
             {"level",
              QJsonObject{{"type", "string"},
                          {"description",
                           "Qt log level: message_debug, message_info, message_warn, "
                           "message_error, message_fatal, message_system, or "
                           "message_location."}}},
             {"text", QJsonObject{{"type", "string"}}},
             {"file", QJsonObject{{"type", "string"}}},
             {"line", QJsonObject{{"type", "integer"}, {"minimum", 1}}}}}};

    const QJsonObject perTestOutcomeSchema{
        {"type", "object"},
        {"properties",
         QJsonObject{
             {"name",
              QJsonObject{{"type", "string"},
                          {"description",
                           "Canonical name in Class::function form."}}},
             {"status",
              QJsonObject{{"type", "string"},
                          {"description",
                           "Outcome string: pass, fail, expected_fail, "
                           "unexpected_pass, skip, blacklisted_*, message_fatal."}}},
             {"failure",
              QJsonObject{{"type", "string"},
                          {"description",
                           "The failure assertion(s) — FAIL!/Actual/Expected/Loc — or the "
                           "skip reason, extracted from the log. Present for failing or "
                           "skipped tests; read this first, it is small and actionable."}}},
             {"warnings",
              QJsonObject{{"type", "string"},
                          {"description",
                           "Warning lines (QWARN/QCRITICAL) extracted from the log, plus "
                           "any warn-level messages. Present only when the test emitted "
                           "warnings. Small — no need to fetch the full log."}}},
             {"message",
              QJsonObject{{"type", "string"},
                          {"description",
                           "Full test log (capped to the tail when very large — see "
                           "'truncated'). Use 'failure' for the assertion; this is the "
                           "surrounding context. Empty for a passing test with no output."}}},
             {"truncated",
              QJsonObject{{"type", "boolean"},
                          {"description",
                           "True if message/failure/messages text was capped; the full "
                           "log is in Qt Creator's Test Results pane."}}},
             {"file", QJsonObject{{"type", "string"}}},
             {"line", QJsonObject{{"type", "integer"}, {"minimum", 1}}},
             {"duration_ms",
              QJsonObject{
                  {"type", "number"},
                  {"description",
                   "Per-function duration in milliseconds, if the test framework "
                   "reported one (GTest, CTest, Catch2). Absent for Qt Test, which "
                   "reports duration at the class level rather than per-function."}}},
             {"messages",
              QJsonObject{
                  {"type", "array"},
                  {"items", perTestMessageItemSchema},
                  {"description",
                   "Full log of qDebug/qInfo/qWarning/qCritical/qFatal messages "
                   "emitted DURING the test function, in arrival order. NOT "
                   "filtered by pass/fail — an empty array means the test (and "
                   "the production code paths it exercised) emitted nothing, "
                   "which is normal, not a bug. For passing tests this is the "
                   "place to look for qInfo() preconditions or sanity-check "
                   "output. Note that Qt Test attributes line numbers to the "
                   "test slot's declaration, not the emit site, so all messages "
                   "in one outcome share a line number."}}}}}};

    // ---- get_test_details (sync read-only) ----
    ToolRegistry::registerTool(
        Tool{}
            .name("get_test_details")
            .title("Get per-test details from the most recent run")
            .description(
                "Per-test details for the named tests from the most recent run. By "
                "default returns only the small, actionable fields — status, 'failure' "
                "(the extracted assertion for a failing test), 'warnings' (any warning "
                "lines), file/line, duration — so a build/test/fix loop never has to "
                "wade through a huge log. Names typically come from run_tests / "
                "get_last_test_results (`failures` / `tests_with_warnings`); unmatched "
                "names appear in `not_found`."
                "\n\n"
                "Pass include:[\"log\"] for the full (tail-capped) output as `message`, "
                "and include:[\"messages\"] for the qDebug/qInfo/... context array. "
                "`truncated` is set when any text was capped; the full log is always in "
                "Qt Creator's Test Results pane.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "names",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description", "Test names to fetch details for."}})
                    .addProperty(
                        "include",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{{"type", "string"},
                                         {"enum", QJsonArray{"log", "messages"}}}},
                            {"description",
                             "Optional heavy fields beyond the default "
                             "status/failure/warnings: \"log\" adds the full (capped) "
                             "output as 'message'; \"messages\" adds the context array. "
                             "Omit to keep the response small."}})
                    .addRequired("names"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "tests",
                        QJsonObject{{"type", "array"}, {"items", perTestOutcomeSchema}})
                    .addProperty(
                        "not_found",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("tests")
                    .addRequired("not_found")),
        wrapChecked({"names", "include"}, [](const QJsonObject &p)
                    -> Utils::Result<QJsonObject> {
            const QJsonValue namesValue = p.value("names");
            if (!namesValue.isArray())
                return ResultError("names must be an array of strings");
            const QJsonArray namesArr = namesValue.toArray();
            if (namesArr.isEmpty())
                return ResultError("names must not be empty");
            QStringList names;
            names.reserve(namesArr.size());
            for (const QJsonValue &v : namesArr) {
                if (!v.isString())
                    return ResultError("names must contain strings only");
                names.append(v.toString());
            }
            bool includeLog = false;
            bool includeMessages = false;
            const QJsonValue includeValue = p.value("include");
            if (!includeValue.isUndefined() && !includeValue.isArray())
                return ResultError("include must be an array of strings");
            for (const QJsonValue &v : includeValue.toArray()) {
                if (!v.isString())
                    return ResultError("include must contain strings only");
                const QString c = v.toString();
                if (c == QLatin1String("log"))
                    includeLog = true;
                else if (c == QLatin1String("messages"))
                    includeMessages = true;
                else
                    return ResultError(QString("Unknown include: %1 (expected log/messages)")
                                           .arg(c));
            }
            return resultsManager().testDetails(names, includeLog, includeMessages);
        }));

    // ---- list_tests (sync read-only) ----
    ToolRegistry::registerTool(
        Tool{}
            .name("list_tests")
            .title("Discover the available tests in the active project")
            .description(
                "Read-only: enumerate every test class Autotest currently knows "
                "about, with its functions. Useful as a discovery step before "
                "calling run_tests — gives exact class and function names to pass "
                "as run_tests({scope: \"named\", names: [\"Class::function\"]}) "
                "without guessing from build artifacts. Each entry carries the "
                "framework label (e.g. \"Qt Test\", \"Google Test\"). "
                "Returns empty if Autotest hasn't finished parsing yet or the "
                "project has no recognized tests.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "tests",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"framework", QJsonObject{{"type", "string"}}},
                                      {"class", QJsonObject{{"type", "string"}}},
                                      {"functions",
                                       QJsonObject{
                                           {"type", "array"},
                                           {"items", QJsonObject{{"type", "string"}}}}},
                                      {"file", QJsonObject{{"type", "string"}}},
                                      {"line", QJsonObject{{"type", "integer"}, {"minimum", 1}}}}}}}})
                    .addProperty("count", QJsonObject{{"type", "integer"}, {"minimum", 0}})
                    .addRequired("tests")
                    .addRequired("count")),
        wrap([](const QJsonObject &) -> QJsonObject {
            TestTreeModel *model = TestTreeModel::instance();
            if (!model)
                return QJsonObject{
                    {"tests", QJsonArray()},
                    {"count", 0},
                    {"error", "Autotest plugin not available"}};

            QJsonArray tests;
            using TT = ITestTreeItem;
            std::function<void(TT *, const QString &)> walk;
            walk = [&](TT *item, const QString &frameworkName) {
                if (!item)
                    return;
                const TT::Type t = item->type();
                if (t == TT::TestCase || t == TT::TestSuite) {
                    QJsonArray functions;
                    bool hasNestedSuites = false;
                    for (int i = 0, n = item->childCount(); i < n; ++i) {
                        TT *child = item->childAt(i);
                        if (!child)
                            continue;
                        const TT::Type ct = child->type();
                        // Only include directly-runnable test items.
                        // TestDataFunction (_data providers) and TestSpecialFunction
                        // (initTestCase / cleanupTestCase) are not independently
                        // addressable — they run implicitly alongside the test.
                        // TestCase children handle frameworks like GTest and Boost
                        // where individual tests are TestCase items under a TestSuite.
                        if (ct == TT::TestFunction || ct == TT::TestCase)
                            functions.append(child->name());
                        else if (ct == TT::TestSuite)
                            hasNestedSuites = true;
                    }
                    // If this item is purely a suite container with no directly-runnable
                    // children (e.g. a Boost nested suite), recurse rather than emitting
                    // an empty entry.
                    if (functions.isEmpty() && hasNestedSuites) {
                        for (int i = 0, n = item->childCount(); i < n; ++i)
                            walk(item->childAt(i), frameworkName);
                        return;
                    }
                    QJsonObject entry;
                    entry.insert("framework", frameworkName);
                    entry.insert("class", item->name());
                    entry.insert("functions", functions);
                    const FilePath fp = item->filePath();
                    if (!fp.isEmpty())
                        entry.insert("file", fp.toUserOutput());
                    if (item->line() > 0)
                        entry.insert("line", item->line());
                    tests.append(entry);
                    return;
                }
                for (int i = 0, n = item->childCount(); i < n; ++i)
                    walk(item->childAt(i), frameworkName);
            };

            auto *root = model->rootItem();
            if (!root)
                return QJsonObject{{"tests", tests}, {"count", 0}};
            for (int i = 0, n = root->childCount(); i < n; ++i) {
                auto *frameworkNode = static_cast<TT *>(root->childAt(i));
                if (frameworkNode)
                    walk(frameworkNode, frameworkNode->name());
            }
            return QJsonObject{{"tests", tests}, {"count", tests.size()}};
        }));
}

} // namespace Autotest::Internal
