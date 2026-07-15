// Copyright (C) 2026 Jeff Heller
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testresultsmanager.h"

#include "testrunner.h"

#include <QJsonArray>
#include <QLoggingCategory>
#include <QSet>
#include <QStringList>

static Q_LOGGING_CATEGORY(mcpTestResults, "qtc.mcpserver.testresults", QtWarningMsg)

namespace Autotest::Internal {

using Autotest::ResultType;
using Autotest::TestResult;
using Autotest::Internal::TestRunner;

static QString resultTypeToString(ResultType type)
{
    switch (type) {
    case ResultType::Pass:
        return QStringLiteral("pass");
    case ResultType::Fail:
        return QStringLiteral("fail");
    case ResultType::ExpectedFail:
        return QStringLiteral("expected_fail");
    case ResultType::UnexpectedPass:
        return QStringLiteral("unexpected_pass");
    case ResultType::Skip:
        return QStringLiteral("skip");
    case ResultType::BlacklistedPass:
        return QStringLiteral("blacklisted_pass");
    case ResultType::BlacklistedFail:
        return QStringLiteral("blacklisted_fail");
    case ResultType::BlacklistedXPass:
        return QStringLiteral("blacklisted_xpass");
    case ResultType::BlacklistedXFail:
        return QStringLiteral("blacklisted_xfail");
    case ResultType::Benchmark:
        return QStringLiteral("benchmark");
    case ResultType::MessageDebug:
        return QStringLiteral("message_debug");
    case ResultType::MessageInfo:
        return QStringLiteral("message_info");
    case ResultType::MessageWarn:
        return QStringLiteral("message_warn");
    case ResultType::MessageFatal:
        return QStringLiteral("message_fatal");
    case ResultType::MessageSystem:
        return QStringLiteral("message_system");
    case ResultType::MessageError:
        return QStringLiteral("message_error");
    case ResultType::MessageLocation:
        return QStringLiteral("message_location");
    case ResultType::TestStart:
        return QStringLiteral("test_start");
    case ResultType::TestEnd:
        return QStringLiteral("test_end");
    default:
        return QStringLiteral("other");
    }
}

// Whether a result counts toward "passed" / "failed" / "skipped" / "fatal"
// buckets in the user-facing summary. Internal/message types don't count.
// MessageFatal is an outcome here because Qt Test uses it as the actual
// failure signal for qFatal()-aborted tests — there is no separate Fail
// emission in that case (see qttestoutputreader.cpp). Autotest's own UI
// counts "fatals" as a separate bucket from "fails"; we mirror that.
static bool isCountable(ResultType type)
{
    switch (type) {
    case ResultType::Pass:
    case ResultType::Fail:
    case ResultType::ExpectedFail:
    case ResultType::UnexpectedPass:
    case ResultType::Skip:
    case ResultType::BlacklistedPass:
    case ResultType::BlacklistedFail:
    case ResultType::BlacklistedXPass:
    case ResultType::BlacklistedXFail:
    case ResultType::MessageFatal:
        return true;
    default:
        return false;
    }
}

// Whether a result is a context message — debug/warn/info/error/system/location
// emitted *during* a test function's execution. We keep these in the stream
// so we can attach them to the surrounding non-pass outcome at snapshot time;
// the AI needs the breadcrumb trail (e.g. the qDebug() values right before a
// qFatal()) to actually understand the failure, not just the fatal line.
static bool isContextMessage(ResultType type)
{
    switch (type) {
    case ResultType::MessageDebug:
    case ResultType::MessageInfo:
    case ResultType::MessageWarn:
    case ResultType::MessageSystem:
    case ResultType::MessageError:
    case ResultType::MessageLocation:
        return true;
    default:
        return false;
    }
}

// Pull lines matching any marker — and their indented continuation (the
// Actual/Expected/Loc block a Qt Test FAIL! prints) — out of a test's full
// output, so failures/warnings are available in a small field without grepping
// the whole (possibly huge) log.
static QString extractMarkedLines(const QString &full, const QStringList &markers)
{
    const QStringList lines = full.split('\n');
    QStringList picked;
    for (int i = 0; i < lines.size(); ++i) {
        bool hit = false;
        for (const QString &m : markers) {
            if (lines.at(i).contains(m)) {
                hit = true;
                break;
            }
        }
        if (!hit)
            continue;
        picked << lines.at(i);
        while (i + 1 < lines.size()
               && (lines.at(i + 1).startsWith(' ') || lines.at(i + 1).startsWith('\t'))) {
            picked << lines.at(++i);
        }
    }
    return picked.join('\n');
}

// Failure / skip / warning markers across frameworks: Qt Test, Google Test,
// ctest. Skip markers are included because summary() lists skips alongside
// failures, so their reason must be reachable in the same small field.
// The markers are the exact prefixes the frameworks print, not just the bare
// words: "ASSERT" alone would also match Qt Creator's "SOFT ASSERT [...]" and
// "SKIP" any line that happens to contain it, both of which tests that go on
// to pass do emit.
static const QStringList &failureMarkers()
{
    static const QStringList m = {
        "FAIL!", "XPASS", "ASSERT: ", "ASSERT failure in", "[  FAILED  ]", "SKIP   : ",
        "***Failed", "***Skipped", "***Exception", "***Timeout", "***Not Run"};
    return m;
}
static const QStringList &warningMarkers()
{
    static const QStringList m = {"QWARN", "QCRITICAL"};
    return m;
}

TestResultsManager::TestResultsManager(QObject *parent)
    : QObject(parent)
{
    connectSignals();
}

void TestResultsManager::connectSignals()
{
    if (m_signalsConnected)
        return;

    TestRunner *runner = TestRunner::instance();
    if (!runner) {
        qCDebug(mcpTestResults) << "TestResultsManager: TestRunner::instance() is null";
        return;
    }

    connect(runner, &TestRunner::testRunStarted, this, &TestResultsManager::onTestRunStarted);
    connect(runner, &TestRunner::testResultReady, this, &TestResultsManager::onTestResultReady);
    connect(runner, &TestRunner::testRunFinished, this, &TestResultsManager::onTestRunFinished);
    connect(runner, &TestRunner::reportSummary, this, &TestResultsManager::onReportSummary);
    connect(runner, &TestRunner::reportDuration, this, &TestResultsManager::onReportDuration);

    m_signalsConnected = true;
    qCDebug(mcpTestResults) << "TestResultsManager: connected to TestRunner signals";
}

bool TestResultsManager::runTests(
    Autotest::TestRunMode mode, const QList<Autotest::ITestConfiguration *> &configs)
{
    TestRunner *runner = TestRunner::instance();
    if (!runner) {
        qCDebug(mcpTestResults) << "TestResultsManager::runTests: TestRunner::instance() is null";
        return false;
    }
    if (runner->isTestRunning() || m_running) {
        qCDebug(mcpTestResults) << "TestResultsManager::runTests: a run is already in progress";
        return false;
    }

    // Pre-clear so callers that observe state before the first signal arrives
    // see a clean slate. testRunStarted will clear again, which is fine.
    m_results.clear();
    m_summary.clear();
    m_durationMs = -1;
    m_buildFailed = false;
    m_running = true;

    runner->runTests(mode, configs);
    return true;
}

void TestResultsManager::onTestRunStarted()
{
    qCDebug(mcpTestResults) << "TestResultsManager: test run started";
    m_results.clear();
    m_summary.clear();
    m_durationMs = -1;
    m_buildFailed = false;
    m_running = true;
}

void TestResultsManager::onTestResultReady(const Autotest::TestResult &result)
{
    const ResultType t = result.result();

    // Flag build-failed: TestRunner emits a single MessageFatal result with the
    // sentinel "Build failed. Canceling test run." (see testrunner.cpp) when
    // the pre-test build doesn't succeed. We check for any MessageFatal that
    // arrives before any countable pass/fail to avoid false positives from
    // unrelated fatal messages mid-run.
    //
    // Invariant: m_results is empty at run start. Both runTests() and
    // onTestRunStarted() clear it explicitly. Future contributors changing
    // either path must preserve this — otherwise a TestStart admitted to
    // m_results before the build-failed MessageFatal would defeat the
    // emptiness check and the build_failed flag would silently never be set.
    if (t == ResultType::MessageFatal && m_results.isEmpty())
        m_buildFailed = true;

    // Filter at receive time, not snapshot time, so memory stays bounded for
    // chatty suites. We keep three classes of result:
    //   1. Outcomes (Pass/Fail/Skip/Fatal/...) — needed for the per-test
    //      results array and the summary counts.
    //   2. Context messages (Debug/Warn/Info/Error/System/Location) — the
    //      breadcrumb trail attached to non-pass outcomes at snapshot time.
    //   3. The build-failed MessageFatal sentinel.
    // Everything else (TestStart/TestEnd, MessageCurrentTest progress pings,
    // Application/Invalid) is dropped — carries no per-AI signal.
    if (!isCountable(t) && !isContextMessage(t)
        && !(t == ResultType::MessageFatal && m_buildFailed))
        return;

    m_results.append(result);
}

void TestResultsManager::onTestRunFinished()
{
    qCDebug(mcpTestResults) << "TestResultsManager: test run finished, results:"
                            << m_results.size();
    m_running = false;
    emit runFinished();
}

void TestResultsManager::recoverFromStuckRun()
{
    if (!m_running)
        return;
    qCWarning(mcpTestResults) << "TestResultsManager: recovering from stuck run "
                                 "(testRunStarted/testRunFinished never fired)";
    m_running = false;
    emit runFinished();
}

void TestResultsManager::onReportSummary(
    const QString &id, const QHash<Autotest::ResultType, int> &summary)
{
    Q_UNUSED(id)
    // Multiple summaries may be reported (e.g. per-framework). Merge them.
    for (auto it = summary.constBegin(); it != summary.constEnd(); ++it)
        m_summary[it.key()] += it.value();
}

void TestResultsManager::onReportDuration(int duration)
{
    m_durationMs = duration;
}

// True for outcomes that the user thinks of as "passing" (green in the test
// pane). Used to categorize tests in summary(). Note: BlacklistedXYZ outcomes
// are deliberately excluded — Qt Test treats them as "blacklisted == result
// does not matter", so we surface them via a separate `blacklisted` count
// rather than collapsing them into pass/fail (see the switch in summary()).
static bool isPassLike(ResultType t)
{
    return t == ResultType::Pass || t == ResultType::ExpectedFail;
}

// True for warn/error/fatal — the levels that signal "this test had a problem
// the user/AI cares about even if it ultimately passed". MessageInfo and
// MessageDebug are routine instrumentation; not surfaced as warnings.
static bool isWarningLike(ResultType t)
{
    return t == ResultType::MessageWarn || t == ResultType::MessageError
           || t == ResultType::MessageFatal || t == ResultType::MessageSystem;
}

// Walk m_results once, calling visitor for each outcome paired with the
// context messages emitted during that outcome's test function.
template<typename Visitor>
static void forEachOutcome(const QList<TestResult> &results, bool buildFailed, Visitor &&visitor)
{
    QList<TestResult> pendingMessages;
    for (const TestResult &r : results) {
        const ResultType t = r.result();
        if (isContextMessage(t)) {
            pendingMessages.append(r);
            continue;
        }
        const bool isBuildFailedSentinel = (t == ResultType::MessageFatal && buildFailed);
        if (!isCountable(t) && !isBuildFailedSentinel)
            continue;
        visitor(r, pendingMessages);
        pendingMessages.clear();
    }
}

QJsonObject TestResultsManager::summary() const
{
    int passed = 0, failed = 0, skipped = 0, fatal = 0, blacklisted = 0, total = 0;
    // Sets guard against duplicate names appearing in failures/testsWithWarnings
    // if the same outputString is emitted more than once (unlikely but defensive).
    QSet<QString> failuresSeen;
    QSet<QString> warningsSeen;
    QJsonArray failures;
    QJsonArray testsWithWarnings;

    forEachOutcome(
        m_results,
        m_buildFailed,
        [&](const TestResult &r, const QList<TestResult> &msgs) {
            const ResultType t = r.result();
            // Skip the build-failed sentinel from per-test counts; it's surfaced
            // via summary.build_failed.
            if (t == ResultType::MessageFatal && m_buildFailed)
                return;
            ++total;
            switch (t) {
            case ResultType::Pass:
            case ResultType::ExpectedFail:
                ++passed;
                break;
            case ResultType::Fail:
            case ResultType::UnexpectedPass:
                ++failed;
                {
                    const QString name = r.outputString(false);
                    if (!failuresSeen.contains(name)) {
                        failuresSeen.insert(name);
                        failures.append(name);
                    }
                }
                break;
            case ResultType::BlacklistedPass:
            case ResultType::BlacklistedFail:
            case ResultType::BlacklistedXPass:
            case ResultType::BlacklistedXFail:
                // Blacklisted outcomes are excluded from passed/failed: Qt Test
                // treats them as "result does not matter for overall result"
                // (see Christian Stenger's review of gerrit 731562). We still
                // count them so total = passed + failed + skipped + fatal +
                // blacklisted, but they do NOT appear in the failures[] array
                // — the project has explicitly told us not to care.
                ++blacklisted;
                break;
            case ResultType::Skip:
                ++skipped;
                {
                    const QString name = r.outputString(false);
                    if (!failuresSeen.contains(name)) {
                        failuresSeen.insert(name);
                        failures.append(name);
                    }
                }
                break;
            case ResultType::MessageFatal:
                ++fatal;
                {
                    const QString name = r.outputString(false);
                    if (!failuresSeen.contains(name)) {
                        failuresSeen.insert(name);
                        failures.append(name);
                    }
                }
                break;
            default:
                break;
            }
            // For passing tests, surface a warning indicator if any of the
            // function's context messages was warn/error/fatal/system. Project
            // policies often forbid warnings on passing tests; this lets the AI
            // identify which tests need cleanup without drilling in everywhere.
            if (isPassLike(t)) {
                for (const TestResult &m : msgs) {
                    if (isWarningLike(m.result())) {
                        const QString name = r.outputString(false);
                        if (!warningsSeen.contains(name)) {
                            warningsSeen.insert(name);
                            testsWithWarnings.append(name);
                        }
                        break;
                    }
                }
            }
        });

    QJsonObject summaryCounts;
    summaryCounts.insert("passed", passed);
    summaryCounts.insert("failed", failed);
    summaryCounts.insert("skipped", skipped);
    summaryCounts.insert("fatal", fatal);
    summaryCounts.insert("blacklisted", blacklisted);
    summaryCounts.insert("total", total);
    summaryCounts.insert("duration_ms", m_durationMs);
    summaryCounts.insert("build_failed", m_buildFailed);

    QString summaryText;
    if (m_buildFailed) {
        summaryText = QStringLiteral("Build failed. No tests run.");
    } else {
        summaryText = QStringLiteral("%1 passed, %2 failed").arg(passed).arg(failed);
        if (fatal > 0)
            summaryText += QStringLiteral(", %1 fatal").arg(fatal);
        summaryText += QStringLiteral(", %1 skipped").arg(skipped);
        if (blacklisted > 0)
            summaryText += QStringLiteral(", %1 blacklisted").arg(blacklisted);
        if (m_durationMs >= 0)
            summaryText += QStringLiteral(" in %1 ms").arg(m_durationMs);
    }

    QJsonObject out;
    out.insert("summary", summaryCounts);
    out.insert("failures", failures);
    out.insert("tests_with_warnings", testsWithWarnings);
    out.insert("summary_text", summaryText);
    return out;
}

QJsonObject TestResultsManager::testDetails(const QStringList &names, bool includeLog,
                                            bool includeMessages) const
{
    // Cap per-field text: a single verbose test (e.g. a CTest run dumping a
    // schema) can be many KB and flood the caller's context. Keep the tail,
    // where the FAIL/Totals summary sits for ctest/Qt Test, and flag it.
    auto cappedText = [](const QString &s, bool *truncated) -> QString {
        static constexpr int kMax = 8192;
        if (s.size() <= kMax)
            return s;
        if (truncated)
            *truncated = true;
        return QStringLiteral("…[showing last %1 of %2 chars; full log in the Test "
                              "Results pane]\n").arg(kMax).arg(s.size()) + s.right(kMax);
    };

    auto messageObject = [&cappedText](const TestResult &m, bool *truncated) {
        QJsonObject obj;
        obj.insert("level", resultTypeToString(m.result()));
        obj.insert("text", cappedText(m.description(), truncated));
        const Utils::FilePath f = m.fileName();
        if (!f.isEmpty())
            obj.insert("file", f.toUserOutput());
        if (m.line() > 0)
            obj.insert("line", m.line());
        return obj;
    };

    auto outcomeObject =
        [&](const TestResult &r, const QList<TestResult> &msgs) {
            QJsonObject obj;
            bool truncated = false;
            obj.insert("name", r.outputString(false));
            obj.insert("status", resultTypeToString(r.result()));

            // Always surface the actionable bits — small and extracted — so a
            // build/test/fix loop never needs the full log: the failure
            // assertion(s) or skip reason, and any warning lines. Skip is here
            // because summary() reports it alongside failures.
            if (r.result() == ResultType::Fail || r.result() == ResultType::UnexpectedPass
                || r.result() == ResultType::MessageFatal || r.result() == ResultType::Skip) {
                const QString fail = extractMarkedLines(r.description(), failureMarkers());
                if (!fail.isEmpty())
                    obj.insert("failure", cappedText(fail, &truncated));
            }
            // A warning can reach us from the log scan and as a warn-level
            // message. Drop only that overlap - a warning the test emitted
            // repeatedly is reported as often as it happened.
            QStringList warns = extractMarkedLines(r.description(), warningMarkers())
                                    .split('\n', Qt::SkipEmptyParts);
            const QSet<QString> fromLog(warns.cbegin(), warns.cend());
            for (const TestResult &m : msgs) {
                if (m.result() != ResultType::MessageWarn || m.description().isEmpty())
                    continue;
                for (const QString &line : m.description().split('\n', Qt::SkipEmptyParts)) {
                    if (!fromLog.contains(line))
                        warns << line;
                }
            }
            if (!warns.isEmpty())
                obj.insert("warnings", cappedText(warns.join('\n'), &truncated));

            // Omit file/line when absent (CTest has neither) rather than null.
            const Utils::FilePath file = r.fileName();
            if (!file.isEmpty())
                obj.insert("file", file.toUserOutput());
            if (r.line() > 0)
                obj.insert("line", r.line());
            const std::optional<QString> dur = r.duration();
            if (dur.has_value()) {
                bool ok = false;
                const double ms = dur->toDouble(&ok);
                if (ok)
                    obj.insert("duration_ms", ms);
            }

            // Full log is opt-in: 'message' is the whole (capped) output,
            // 'messages' the context array — kept out of the default response so
            // the caller is never forced to receive a huge file.
            if (includeLog)
                obj.insert("message", cappedText(r.description(), &truncated));
            if (includeMessages) {
                QJsonArray msgArr;
                for (const TestResult &m : msgs)
                    msgArr.append(messageObject(m, &truncated));
                obj.insert("messages", msgArr);
            }
            if (truncated)
                obj.insert("truncated", true);
            return obj;
        };

    // Build a set for fast lookup; track which names were matched so we can
    // report not_found accurately even if a name matches multiple outcomes.
    const QSet<QString> wanted(names.begin(), names.end());
    QSet<QString> matched;
    QJsonArray tests;

    forEachOutcome(
        m_results,
        m_buildFailed,
        [&](const TestResult &r, const QList<TestResult> &msgs) {
            const QString cname = r.outputString(false);
            if (!wanted.contains(cname))
                return;
            matched.insert(cname);
            tests.append(outcomeObject(r, msgs));
        });

    QJsonArray notFound;
    for (const QString &n : names) {
        if (!matched.contains(n))
            notFound.append(n);
    }

    QJsonObject out;
    out.insert("tests", tests);
    out.insert("not_found", notFound);
    return out;
}

} // namespace Autotest::Internal
