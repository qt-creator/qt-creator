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

#include <QDateTime>
#include <QElapsedTimer>
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

// Required on every reply, so errors answer in the tool's own shape.
static QJsonObject placeholderSummary()
{
    return QJsonObject{
        {"passed", 0},
        {"failed", 0},
        {"skipped", 0},
        {"fatal", 0},
        {"blacklisted", 0},
        {"total", 0},
        {"duration_ms", -1},
        {"build_failed", false}};
}

static QJsonObject noResultBody(const QString &reason, const QString &message)
{
    return QJsonObject{
        {"finished", true},
        {"reason", reason},
        {"summary", placeholderSummary()},
        {"failures", QJsonArray{}},
        {"tests_with_warnings", QJsonArray{}},
        {"summary_text", message}};
}

// Kept under the request timeout of typical MCP clients, maximum included: a
// longer wait means the transport drops the session, which cancels the run.
constexpr qint64 defaultRunWaitMs = 45000;
constexpr qint64 minRunWaitMs = 1000;
constexpr qint64 maxRunWaitMs = 55000;

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
    qint64 waitMs = defaultRunWaitMs;
    quint64 runId = 0; // 0 means "no run named"
};

// Types are checked here too: toString()/toArray() turn a wrong type into the
// default, and for scope that default is a silent full run.
static Utils::Result<RunArgs> readRunArgs(const QJsonObject &p)
{
    if (const Utils::Result<> known
        = rejectUnknownArgs(p, {"scope", "names", "mode", "wait_ms", "run_id"});
        !known) {
        return ResultError(known.error());
    }

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

    if (!p.value("run_id").isUndefined() && !p.value("run_id").isNull()
        && (p.contains("scope") || p.contains("names"))) {
        return ResultError("run_id collects a run that already exists, so it cannot be "
                           "combined with scope or names. Pass run_id alone, or drop it "
                           "to start the run you are describing.");
    }

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

    const auto readNumber = [&p](const QString &key, qint64 fallback) -> Utils::Result<qint64> {
        const QJsonValue v = p.value(key);
        if (v.isUndefined() || v.isNull())
            return fallback;
        if (!v.isDouble())
            return ResultError(QString("%1 must be a number").arg(key));
        return qint64(v.toDouble());
    };
    const Utils::Result<qint64> waitMs = readNumber("wait_ms", defaultRunWaitMs);
    if (!waitMs)
        return ResultError(waitMs.error());
    args.waitMs = qBound(minRunWaitMs, *waitMs, maxRunWaitMs);
    const Utils::Result<qint64> runId = readNumber("run_id", 0);
    if (!runId)
        return ResultError(runId.error());
    if (*runId < 0)
        return ResultError("run_id must not be negative");
    args.runId = quint64(*runId);

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

    // Lets a bounded wait be collected by run_id and a second call attach
    // instead of failing. Tests-pane runs are adopted too.
    struct TestRunSlot
    {
        bool inProgress = false;
        bool finished = false; // generation's summary is the current one
        quint64 generation = 0;
        QElapsedTimer elapsed;
        QJsonObject summary; // snapshot taken when generation ended
        QJsonArray buildIssues; // from that same moment, not the current build
    };
    // Leaked: that destructor would run at exit(), on memory this plugin has
    // already released.
    static TestRunSlot &runSlot = *new TestRunSlot;

    // Seeded from the process start: an id held across a restart must not
    // match a different run.
    static quint64 lastRunId = quint64(QDateTime::currentMSecsSinceEpoch());

    // Idempotent, so a run this tool launched keeps its id when runStarted()
    // follows.
    static const auto observeRun = [] {
        if (!runSlot.inProgress) {
            runSlot.inProgress = true;
            runSlot.finished = false;
            runSlot.summary = {};
            runSlot.generation = ++lastRunId;
            runSlot.elapsed.start();
        }
        return runSlot.generation;
    };

    // Every run takes an id, so a superseded one fails to match instead of
    // collecting a later run's results.
    static const QMetaObject::Connection runStartedConnection = QObject::connect(
        &resultsManager(), &TestResultsManager::runStarted, &resultsManager(), [] {
            observeRun();
        });
    Q_UNUSED(runStartedConnection)

    // summary() is the most recent run's: one starting in between would
    // otherwise be reported as this one's.
    static const QMetaObject::Connection runSlotConnection = QObject::connect(
        &resultsManager(), &TestResultsManager::runFinished, &resultsManager(), [] {
            if (!runSlot.inProgress)
                return;
            runSlot.summary = resultsManager().summary();
            runSlot.buildIssues
                = runSlot.summary.value("summary").toObject().value("build_failed").toBool()
                      ? issuesManager.getBuildIssues().value("issues").toArray()
                      : QJsonArray{};
            runSlot.inProgress = false;
            runSlot.finished = true;
        });
    Q_UNUSED(runSlotConnection)

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

    // run_tests answers before the run necessarily ends, so it needs the fields
    // that say so.
    const auto runTestsOutputSchema
        = Tool::OutputSchema{testSummaryOutputSchema}
              .addProperty(
                  "finished",
                  QJsonObject{
                      {"type", "boolean"},
                      {"description",
                       "Whether this response is a final answer. Check it first: when "
                       "false the run is still going and the counts below are "
                       "placeholders, not an all-zero result."}})
              .addProperty(
                  "reason",
                  QJsonObject{
                      {"type", "string"},
                      {"description",
                       "Why no result yet: \"still_running\" (the wait_ms budget ran out, "
                       "the run continues), \"discovery_pending\" (still waiting for test "
                       "discovery, nothing started yet) or \"unknown_run_id\". On other "
                       "errors: \"invalid_arguments\", \"not_found\" (no test matched), "
                       "\"start_failed\", \"cancelled\", \"no_run_started\" or "
                       "\"superseded\" (another run replaced this one's results). "
                       "\"joined_existing_run\" replaces the reasons above whenever this "
                       "call joined a run that was already going: on a finished reply the "
                       "results are that run's, and on an unfinished one the run_id will "
                       "collect them, so in both cases they are not necessarily the "
                       "scope/names this call asked for. \"call_cancelled\" means this "
                       "call was cancelled; if it carries a run_id that run is still "
                       "going."}})
              .addProperty(
                  "run_id",
                  QJsonObject{
                      {"type", "integer"},
                      {"description",
                       "Identifies the run this response is about. Pass it back as run_id "
                       "to keep waiting for that same run, or to collect its result once "
                       "it has finished."}})
              .addProperty(
                  "elapsed_ms",
                  QJsonObject{
                      {"type", "integer"},
                      {"minimum", 0},
                      {"description", "How long the run has been going. Set only while it "
                                      "is still running."}})
              .addRequired("finished");

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
                "instead of clobbering it."
                "\n\n"
                "Read `finished` first. When it is true the summary is this run's result. "
                "When it is false nothing failed — the run simply has not ended yet, and "
                "the response carries a run_id, elapsed_ms and a reason of "
                "\"still_running\", or \"joined_existing_run\" if the run it is waiting "
                "for is one that was already going. "
                "Call run_tests again with that run_id to keep waiting, and repeat until "
                "finished is true. Do not start a second run and do not sleep between "
                "calls: each call does the waiting for you."
                "\n\n"
                "A run already going — whether this tool started it or the user hit Run in "
                "the Tests pane — is joined rather than refused, so a repeated call cannot "
                "launch a competing run.")
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
                             "much slower; use it for narrowing in on a known failure."}})
                    .addProperty(
                        "wait_ms",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "How long to block before returning reason:\"still_running\", "
                             "in milliseconds (default 45000). Clamped to 1000-55000: a "
                             "longer wait outlives the request timeout of typical clients, "
                             "which drops the session and cancels the run. Poll with run_id "
                             "instead of asking for a longer wait."}})
                    .addProperty(
                        "run_id",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Attach to the run with this id instead of starting one. Use "
                             "the run_id from a previous still_running response."}}))
            .outputSchema(runTestsOutputSchema)
            .annotations(ToolAnnotations{}.readOnlyHint(false)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject p = params.argumentsAsObject();

            // Before startTask(): a call rejected here never runs, and a task
            // left behind would be reported completed by its own heartbeat.
            const Utils::Result<RunArgs> args = readRunArgs(p);
            if (!args) {
                toolInterface.finish(
                    CallToolResult{}
                        .structuredContent(noResultBody("invalid_arguments", args.error()))
                        .isError(true));
                return ResultOk;
            }
            const QString scope = args->scope;
            const QString mode = args->mode;
            const QStringList names = args->names;
            const qint64 waitMs = args->waitMs;
            const quint64 runId = args->runId;

            struct State
            {
                bool cancelRequested = false;
                bool waitingForParser = false;
                bool boundedOut = false;     // wait_ms expired, work continues
                bool taskOver = false;       // the task has been answered; don't notify
                bool startedRun = false;     // this call launched the run (may cancel it)
                bool joinedOther = false;    // joined a run this call did not ask for
                quint64 generation = 0;      // run_id this call is bound to
                std::optional<QString> resolveError;
                QString errorReason; // the reason field that goes with it
                std::optional<Schema::TaskStatus> finalStatus;
                ToolInterface::TaskProgressNotify notify;

                bool finished() const { return finalStatus.has_value(); }

                // The only way to end the task, so the heartbeat cannot report a
                // status the client was never notified with. A cancelled run ends
                // through runFinished, indistinguishable from a normal end.
                void finish(Schema::TaskStatus status, const QString &message)
                {
                    const bool cancelled = cancelRequested
                                           && status == Schema::TaskStatus::completed;
                    finalStatus = cancelled ? Schema::TaskStatus::cancelled : status;
                    if (notify && !taskOver)
                        notify(*finalStatus, cancelled ? "Cancelled" : message, std::nullopt);
                }
                QElapsedTimer waited;

                // Snapshotted when known, not read from the process-wide slot at
                // reply time: in task mode that is tasks/result, by which point
                // another run may have claimed it.
                bool haveResult = false;
                QJsonObject result;
                QJsonArray buildIssues;
                bool boundedRunning = false;
                qint64 boundedElapsedMs = 0;
            };
            auto state = std::make_shared<State>();
            // Waiting for discovery counts against the budget too.
            state->waited.start();

            // startTask is always called synchronously so the client gets a task
            // ID immediately.  The actual resolve+run is deferred via startRun
            // if the parser is still scanning (see below).
            using namespace std::chrono_literals;
            const Utils::Result<ToolInterface::TaskProgressNotify> task = toolInterface.startTask(
                500ms,
                [state, waitMs](Schema::Task t) -> Schema::Task {
                    if (state->finished()) {
                        letTaskDieIn(t, 1min);
                        // Outlives the notify() that set the outcome.
                        return t.status(*state->finalStatus);
                    }
                    if (state->waited.elapsed() >= waitMs) {
                        state->boundedOut = true;
                        state->taskOver = true;
                        // The parser handlers and watchdog bail on finished, so a
                        // bounded-out call cannot start a run later.
                        state->finalStatus = Schema::TaskStatus::completed;
                        state->boundedRunning = runSlot.inProgress && state->generation != 0
                                                && runSlot.generation == state->generation;
                        state->boundedElapsedMs = state->boundedRunning
                                                      ? runSlot.elapsed.elapsed()
                                                      : state->waited.elapsed();
                        letTaskDieIn(t, 1min);
                        t.statusMessage("Still running");
                        return t.status(Schema::TaskStatus::completed);
                    }
                    const char *msg = state->cancelRequested  ? "Cancelling tests..."
                                    : state->waitingForParser ? "Waiting for test discovery..."
                                                              : "Running tests...";
                    t.statusMessage(QString::fromLatin1(msg));
                    return t.status(Schema::TaskStatus::working);
                },
                [state]() -> Utils::Result<CallToolResult> {
                    if (state->resolveError) {
                        return CallToolResult{}
                            .structuredContent(
                                noResultBody(state->errorReason, *state->resolveError))
                            .isError(true);
                    }
                    if (state->boundedOut) {
                        // Not an error: the run is alive, the counts are
                        // placeholders.
                        const bool running = state->boundedRunning;
                        const qint64 elapsedMs = state->boundedElapsedMs;
                        QJsonObject body{
                            {"finished", false},
                            {"reason", running ? "still_running" : "discovery_pending"},
                            {"elapsed_ms", elapsedMs},
                            {"summary", placeholderSummary()},
                            {"failures", QJsonArray{}},
                            {"tests_with_warnings", QJsonArray{}},
                        };
                        if (running && state->joinedOther) {
                            // The mismatch is known here and nowhere later: a
                            // call collecting by run_id passes no selection, so
                            // it cannot tell the run was not the one asked for.
                            body["reason"] = "joined_existing_run";
                            body["run_id"] = qint64(state->generation);
                            body["summary_text"]
                                = QString("Joined a run already in progress, not the "
                                          "selection requested; still running after %1 ms. "
                                          "Call run_tests again with run_id:%2 to collect "
                                          "it, then again to run what you asked for.")
                                      .arg(elapsedMs)
                                      .arg(state->generation);
                        } else if (running) {
                            body["run_id"] = qint64(state->generation);
                            body["summary_text"]
                                = QString("Tests still running after %1 ms. Nothing has "
                                          "failed — call run_tests again with run_id:%2 to "
                                          "keep waiting.")
                                      .arg(elapsedMs)
                                      .arg(state->generation);
                        } else {
                            body["summary_text"]
                                = QString("Still waiting for test discovery after %1 ms; no "
                                          "run has started. Call run_tests again to keep "
                                          "waiting.")
                                      .arg(elapsedMs);
                        }
                        return CallToolResult{}.isError(false).structuredContent(body);
                    }
                    if (!state->haveResult) {
                        if (state->cancelRequested) {
                            // Only a run this call started is cancelled with it;
                            // an attach leaves it going, so say which happened.
                            if (!state->startedRun && state->generation != 0) {
                                QJsonObject body = noResultBody(
                                    "call_cancelled",
                                    "This call was cancelled. The test run it was "
                                    "waiting for is still going -- call run_tests "
                                    "again with its run_id to collect it.");
                                body["finished"] = false;
                                body["run_id"] = qint64(state->generation);
                                return CallToolResult{}.structuredContent(body).isError(false);
                            }
                            if (!state->startedRun) {
                                return CallToolResult{}
                                    .structuredContent(noResultBody(
                                        "call_cancelled",
                                        "This call was cancelled before any test run "
                                        "started. Nothing is running; call run_tests "
                                        "again to start one."))
                                    .isError(true);
                            }
                            return CallToolResult{}
                                .structuredContent(noResultBody(
                                    "cancelled",
                                    "The test run was cancelled before it produced "
                                    "results."))
                                .isError(true);
                        }
                        if (state->generation == 0) {
                            return CallToolResult{}
                                .structuredContent(noResultBody(
                                    "no_run_started",
                                    "No test run was started, so there is no result. Call "
                                    "run_tests again."))
                                .isError(true);
                        }
                        if (runSlot.inProgress && runSlot.generation == state->generation) {
                            QJsonObject body = noResultBody(
                                "still_running",
                                QString("Run %1 has not produced results yet. Call run_tests "
                                        "again with run_id:%1 to keep waiting.")
                                    .arg(state->generation));
                            body["finished"] = false;
                            body["run_id"] = qint64(state->generation);
                            return CallToolResult{}.structuredContent(body).isError(false);
                        }
                        // summary() is whatever ran most recently, not this run.
                        QJsonObject body = noResultBody(
                            "superseded",
                            QString("The results of run %1 are no longer available — another "
                                    "run replaced them. Call run_tests again to re-run.")
                                .arg(state->generation));
                        body["run_id"] = qint64(state->generation);
                        return CallToolResult{}.structuredContent(body).isError(true);
                    }
                    QJsonObject result = state->result;
                    result["finished"] = true;
                    if (state->generation != 0)
                        result["run_id"] = qint64(state->generation);
                    if (state->joinedOther) {
                        result["reason"] = "joined_existing_run";
                        result["summary_text"]
                            = QString("%1 (from a run already in progress, not the selection "
                                      "requested — run_tests again once it is done to run "
                                      "exactly what you asked for)")
                                  .arg(result.value("summary_text").toString());
                    }
                    // Fold build issues inline when the build that gates the
                    // test run failed. Saves the AI a separate list_issues call
                    // to find out WHY the build broke.
                    if (!state->buildIssues.isEmpty())
                        result.insert("build_issues", state->buildIssues);
                    return CallToolResult{}.isError(false).structuredContent(result);
                },
                [state]() {
                    state->cancelRequested = true;
                    // An attach must not abort the user's run, nor a superseded
                    // id whatever is running now. Nor may a call that already
                    // answered "still running" abort what it told the caller to
                    // poll for.
                    if (state->startedRun && !state->boundedOut && runSlot.inProgress
                        && runSlot.generation == state->generation) {
                        cancelTestRun();
                        return; // runFinished settles the status
                    }
                    // Nothing was cancelled but this call, and no signal will
                    // come for it: settle now, or the heartbeat walks the
                    // status back from cancelled to working.
                    if (!state->finished())
                        state->finish(Schema::TaskStatus::cancelled, "Cancelled");
                },
                progressToken(params));

            if (!task) {
                toolInterface.finish(
                    CallToolResult{}
                        .structuredContent(noResultBody("task_failed", task.error()))
                        .isError(true));
                return ResultOk;
            }

            state->notify = *task;

            auto awaitFinish = [state]() -> std::shared_ptr<QMetaObject::Connection> {
                auto conn = std::make_shared<QMetaObject::Connection>();
                *conn = QObject::connect(
                    &resultsManager(),
                    &TestResultsManager::runFinished,
                    &resultsManager(),
                    [state, conn]() {
                        QObject::disconnect(*conn);
                        if (state->finished())
                            return;
                        if (runSlot.finished && runSlot.generation == state->generation) {
                            state->result = runSlot.summary;
                            state->buildIssues = runSlot.buildIssues;
                            state->haveResult = true;
                        }
                        state->finish(Schema::TaskStatus::completed, "Tests finished");
                    });
                return conn;
            };

            auto startRun = [state, awaitFinish](ResolvedTestRun resolved) {
                state->waitingForParser = false;

                if (state->cancelRequested) {
                    qDeleteAll(resolved.configs); // owned here
                    state->finish(Schema::TaskStatus::cancelled, "Cancelled");
                    return;
                }

                if (resultsManager().isRunning()) {
                    qDeleteAll(resolved.configs); // owned here
                    state->generation = observeRun();
                    state->joinedOther = true;
                    awaitFinish();
                    return;
                }

                auto conn = awaitFinish();
                // Claim before the run starts, so runFinished sees a slot.
                const quint64 previousGeneration = runSlot.generation;
                const bool previousFinished = runSlot.finished;
                const QJsonObject previousSummary = runSlot.summary;
                state->generation = observeRun();
                const bool mintedSlot = runSlot.generation != previousGeneration;

                if (!resultsManager().runTests(resolved.mode, resolved.configs)) {
                    // Claiming bumped the generation and cleared the summary, so
                    // flags alone would strand the previous run_id's results.
                    if (mintedSlot) {
                        runSlot.inProgress = false;
                        runSlot.generation = previousGeneration;
                        runSlot.finished = previousFinished;
                        runSlot.summary = previousSummary;
                    }
                    qDeleteAll(resolved.configs);
                    QObject::disconnect(*conn);
                    state->resolveError = "Failed to start test run (already in progress?)";
                    state->errorReason = "start_failed";
                    state->finish(Schema::TaskStatus::failed, "Failed to start");
                    return;
                }

                state->startedRun = true;

                // Keyed to the run, not to this call: a bounded-out or cancelled
                // call leaves the run going, and it is the run that can wedge.
                QTimer::singleShot(5000, &resultsManager(), [generation = state->generation]() {
                    if (!runSlot.inProgress || runSlot.generation != generation)
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
                    state->errorReason = "not_found";
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
                // A parser that stops without emitting - project closed mid-scan,
                // say - would leave the task waiting for good.
                auto *watchdog = new QTimer(parser);
                watchdog->setInterval(parserPollInterval);
                QObject::connect(
                    watchdog, &QTimer::timeout, parser,
                    [watchdog, stopWaiting, resolveAndRun, state] {
                        if (state->finished() || !state->waitingForParser) {
                            stopWaiting();
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

            // A named run is collected or joined, never restarted.
            if (runId != 0) {
                if (runSlot.generation != runId) {
                    state->resolveError
                        = QString("No test run with id %1 is known: a newer run has "
                                  "replaced it, or it predates a restart. Call run_tests "
                                  "without run_id to start one.")
                              .arg(runId);
                    state->errorReason = "unknown_run_id";
                    state->finish(Schema::TaskStatus::failed, "Unknown run_id");
                    return ResultOk;
                }
                state->generation = runId;
                if (runSlot.finished) {
                    state->result = runSlot.summary;
                    state->buildIssues = runSlot.buildIssues;
                    state->haveResult = true;
                    state->finish(Schema::TaskStatus::completed, "Tests finished");
                    return ResultOk;
                }
                awaitFinish();
                return ResultOk;
            }

            // Joined rather than refused: runTests() would fail, and the caller
            // wants that run's result anyway.
            if (resultsManager().isRunning()) {
                state->generation = observeRun();
                state->joinedOther = true;
                awaitFinish();
                return ResultOk;
            }

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
                state->errorReason = "not_found";
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
