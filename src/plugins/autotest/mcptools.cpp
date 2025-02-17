// Copyright (C) 2026 Jeff Heller
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testresultsmanager.h"

#include "autotestconstants.h"
#include "testconfiguration.h"
#include "testcodeparser.h"
#include "testrunner.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <projectexplorer/issuesmanager.h>

#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTimer>

using namespace Mcp;
using namespace Utils;

namespace Schema = Mcp::Schema;

Q_LOGGING_CATEGORY(mcpAutotest, "qtc.autotest.mcptools", QtWarningMsg)

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

static Utils::Result<ResolvedTestRun> resolveTestRun(
    const QString &scope, const QStringList &names, const QString &mode)
{
    qCDebug(mcpAutotest) << "resolveTestRun: scope=" << scope << " names=" << names
                         << " mode=" << mode;

    TestTreeModel *model = TestTreeModel::instance();
    if (!model)
        return ResultError("Autotest plugin not available");

    TestRunMode runMode;
    const QString m = mode.isEmpty() ? QStringLiteral("run") : mode;
    if (m == QLatin1String("run"))
        runMode = TestRunMode::Run;
    else if (m == QLatin1String("debug"))
        runMode = TestRunMode::Debug;
    else
        return ResultError(QString("Unknown mode: %1 (expected run/debug)").arg(m));

    QList<ITestConfiguration *> configs;
    const QString s = scope.isEmpty() ? QStringLiteral("all") : scope;
    if (s == QLatin1String("selected")) {
        configs = model->getSelectedTests();
    } else if (s == QLatin1String("failed")) {
        configs = model->getFailedTests();
    } else if (s == QLatin1String("all")) {
        configs = model->getAllTestCases(runMode);
    } else if (s == QLatin1String("named")) {
        if (names.isEmpty())
            return ResultError("scope=named requires a non-empty names array");

        auto lookupByName = [model](const QString &qualifiedName) -> QList<ITestTreeItem *> {
            const int sep = qualifiedName.indexOf(QStringLiteral("::"));
            if (sep < 0)
                return model->testItemsByName(qualifiedName);
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
        for (const QList<ITestTreeItem *> &items : itemsPerName) {
            for (ITestTreeItem *item : items) {
                if (ITestConfiguration *cfg = item->asConfiguration(runMode))
                    configs.append(cfg);
            }
        }
    } else {
        return ResultError(QString("Unknown scope: %1 (expected all/selected/failed/named)").arg(s));
    }

    if (configs.isEmpty())
        return ResultError(QString("No tests to run for scope: %1").arg(s));

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

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    const auto wrap = [](SimplifiedCallback &&callback) -> Server::ToolCallback {
        return [callback](
                   const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject result = callback(params.argumentsAsObject());
            return CallToolResult{}.isError(false).structuredContent(result);
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
                "test names that emitted warnings. Equivalent to clicking Run (or Debug, "
                "with mode='debug') in the Tests pane. Returns once the run finishes. "
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
                             "(default). 'selected' runs whatever is checked in the Tests "
                             "pane. 'failed' re-runs the tests that failed in the previous "
                             "run. 'named' runs only the tests in the `names` array."}})
                    .addProperty(
                        "names",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description",
                             "Test names to run when scope='named'. Names typically come "
                             "from `failures` or `tests_with_warnings` in a previous "
                             "summary, or from list_tests. Names must already be present "
                             "in Autotest's current model — if a name is not found, "
                             "call reconfigure first to trigger a re-parse (needed after "
                             "adding or renaming test functions). Ignored for other scopes."}})
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
            const QString scope = p.value("scope").toString("all");
            const QString mode = p.value("mode").toString("run");
            QStringList names;
            const QJsonArray namesArr = p.value("names").toArray();
            names.reserve(namesArr.size());
            for (const QJsonValue &v : namesArr)
                names.append(v.toString());

            struct State
            {
                bool finished = false;
                bool cancelRequested = false;
                bool waitingForParser = false;
                std::optional<QString> resolveError;
            };
            auto state = std::make_shared<State>();

            // startTask is always called synchronously so the client gets a task
            // ID immediately.  The actual resolve+run is deferred via startRun
            // if the parser is still scanning (see below).
            using namespace std::chrono_literals;
            const Utils::Result<ToolInterface::TaskProgressNotify> task = toolInterface.startTask(
                500ms,
                [state](Schema::Task t) -> Schema::Task {
                    if (state->finished) {
                        letTaskDieIn(t, 1min);
                        return t.status(Schema::TaskStatus::completed);
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
                        static ProjectExplorer::IssuesManager issuesManager;
                        const QJsonObject issuesData = issuesManager.getCurrentIssues();
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

            const ToolInterface::TaskProgressNotify notify = *task;

            // startRun resolves the test tree and kicks off the actual run.
            // Called directly when the parser is idle, or deferred to
            // parsingFinished/parsingFailed when a scan is in progress.
            auto startRun = [scope, names, mode, state, notify]() {
                state->waitingForParser = false;

                if (state->cancelRequested) {
                    state->finished = true;
                    if (notify)
                        notify(Schema::TaskStatus::cancelled, "Cancelled", std::nullopt);
                    return;
                }

                const Utils::Result<ResolvedTestRun> resolved =
                    resolveTestRun(scope, names, mode);
                if (!resolved) {
                    state->resolveError = resolved.error();
                    state->finished = true;
                    if (notify)
                        notify(Schema::TaskStatus::failed, "Error", std::nullopt);
                    return;
                }

                auto conn = std::make_shared<QMetaObject::Connection>();
                *conn = QObject::connect(
                    &resultsManager(),
                    &TestResultsManager::runFinished,
                    &resultsManager(),
                    [state, notify, conn]() {
                        QObject::disconnect(*conn);
                        if (state->finished)
                            return;
                        state->finished = true;
                        if (notify)
                            notify(Schema::TaskStatus::completed, "Tests finished", std::nullopt);
                    });

                if (!resultsManager().runTests(resolved->mode, resolved->configs)) {
                    qDeleteAll(resolved->configs);
                    QObject::disconnect(*conn);
                    state->resolveError = "Failed to start test run (already in progress?)";
                    state->finished = true;
                    if (notify)
                        notify(Schema::TaskStatus::failed, "Failed to start", std::nullopt);
                    return;
                }

                QTimer::singleShot(5000, &resultsManager(), [state]() {
                    if (state->finished)
                        return;
                    if (!resultsManager().isRunning())
                        return;
                    TestRunner *runner = TestRunner::instance();
                    if (!runner || !runner->isTestRunning())
                        resultsManager().recoverFromStuckRun();
                });
            };

            // If the test-code parser is mid-scan or has a scan scheduled (e.g.
            // after reconfigure+build triggered a 1-second delayed rescan), defer
            // startRun until parsing is fully settled so every test class is
            // visible in the tree. isParsingOrScheduled() covers both the actively
            // parsing case and the "single-shot timer pending" case.
            // Qt::QueuedConnection guarantees our callback is posted to the event
            // loop after TestTreeModel::sweep() — which is connected to the same
            // signals with QueuedConnection in the model constructor and was
            // therefore connected first.
            //
            // isParsingOrScheduled() is read here — after startTask() — to
            // eliminate the window where parsingFinished could fire between an
            // earlier snapshot and the connection being installed, which would
            // leave startRun permanently deferred.
            //
            // The connections are kept alive across multiple parsingFinished
            // firings: a build can trigger a rapid succession of scans (initial
            // file sweep → 1-second delayed rescan), so parsingFinished may fire
            // while isParsingOrScheduled() is still true.  onParsingDone re-checks
            // after each firing and only calls startRun() once the parser has
            // fully converged.
            TestTreeModel *model = TestTreeModel::instance();
            if (model && model->parser()->isParsingOrScheduled()) {
                state->waitingForParser = true;
                qCDebug(mcpAutotest)
                    << "run_tests: parser active or scan scheduled, deferring until parsingFinished";
                TestCodeParser *parser = model->parser();
                auto connFinished = std::make_shared<QMetaObject::Connection>();
                auto connFailed = std::make_shared<QMetaObject::Connection>();
                auto onParsingDone = [connFinished, connFailed, startRun, state, notify]() {
                    if (state->cancelRequested) {
                        QObject::disconnect(*connFinished);
                        QObject::disconnect(*connFailed);
                        state->waitingForParser = false;
                        state->finished = true;
                        if (notify)
                            notify(Schema::TaskStatus::cancelled, "Cancelled", std::nullopt);
                        return;
                    }
                    // Re-check after each parsingFinished: a build can trigger a
                    // rapid series of scans.  Stay deferred until isParsingOrScheduled()
                    // is false so the full model is available before resolving tests.
                    TestTreeModel *liveModel = TestTreeModel::instance();
                    if (liveModel && liveModel->parser()->isParsingOrScheduled()) {
                        qCDebug(mcpAutotest)
                            << "run_tests: parsingFinished fired but another scan is pending, "
                               "waiting for full convergence";
                        return; // connections stay installed
                    }
                    // Disconnect both before startRun() to prevent double-dispatch if
                    // the other signal fires in the same event-loop tick.
                    QObject::disconnect(*connFinished);
                    QObject::disconnect(*connFailed);
                    startRun();
                };
                *connFinished = QObject::connect(
                    parser, &TestCodeParser::parsingFinished,
                    model, onParsingDone, Qt::QueuedConnection);
                *connFailed = QObject::connect(
                    parser, &TestCodeParser::parsingFailed,
                    model, onParsingDone, Qt::QueuedConnection);
            } else {
                startRun();
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
             {"message",
              QJsonObject{{"type", "string"},
                          {"description",
                           "Failure description for non-pass outcomes; empty for "
                           "passing tests."}}},
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
                "Full details for the named tests from the most recent run "
                "— status, failure message, file/line, duration, and the full log of "
                "qDebug/qInfo/qWarning/qCritical/qFatal messages emitted during that "
                "test function. Names typically come from the `failures` or "
                "`tests_with_warnings` arrays returned by run_tests / "
                "get_last_test_results, but any test name from the most recent run is "
                "valid. Names that don't match any outcome appear in `not_found`."
                "\n\n"
                "The `messages` array is NEVER filtered by pass/fail status — every "
                "outcome gets its full breadcrumb trail. An empty `messages[]` means "
                "the test (and the production code it exercised) didn't emit anything, "
                "which is normal. This tool is useful even for passing tests when you "
                "want to verify qInfo preconditions or read sanity-check output.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "names",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description", "Test names to fetch details for."}})
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
        wrap([](const QJsonObject &p) {
            QStringList names;
            const QJsonArray namesArr = p.value("names").toArray();
            names.reserve(namesArr.size());
            for (const QJsonValue &v : namesArr)
                names.append(v.toString());
            return resultsManager().testDetails(names);
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
