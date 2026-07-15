// Copyright (C) 2026 Jeff Heller
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "autotestconstants.h"
#include "testresult.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>

namespace Autotest {
class ITestConfiguration;
}

namespace Autotest::Internal {

class TestResultsManager : public QObject
{
    Q_OBJECT

public:
    explicit TestResultsManager(QObject *parent = nullptr);
    ~TestResultsManager() override = default;

    // Kicks off a test run via Autotest::Internal::TestRunner. Returns false if
    // a run is already in progress.
    bool runTests(Autotest::TestRunMode mode, const QList<Autotest::ITestConfiguration *> &configs);

    bool isRunning() const { return m_running; }

    // Recovery hook for the case where TestRunner accepts our runTests()
    // call but never emits testRunStarted/testRunFinished — e.g. configs
    // silently rejected by precheckTestConfigurations. Caller (a 5-second
    // QTimer::singleShot watchdog set up in the run_tests tool handler in
    // mcpcommands.cpp::registerCommands) invokes this when it detects
    // that the underlying runner is no longer running but our m_running
    // flag is still true. Clears state and emits runFinished so the task
    // heartbeat in the tool handler can deliver the (empty) snapshot and
    // the next run isn't blocked.
    void recoverFromStuckRun();

    // Compact summary of the most recent run. Returns counts plus categorized
    // name lists: `failures` (tests that failed/fataled/skipped) and
    // `tests_with_warnings` (passing tests that emitted any qWarning/qCritical
    // /qFatal — useful for projects that forbid warnings on passing tests).
    // Per-test details and message logs are NOT included here — call
    // testDetails() with the test names you want to drill into. This keeps
    // the response tiny regardless of suite size.
    QJsonObject summary() const;

    // Detailed view of specific tests by name. Always returns the small,
    // actionable fields (status, extracted failure/warnings, file/line,
    // duration); the full log (message) and context messages array are only
    // included when includeLog / includeMessages are set, so the default
    // response stays tiny. Names with no matching outcome appear in `not_found`.
    QJsonObject testDetails(const QStringList &names, bool includeLog,
                            bool includeMessages) const;

signals:
    void runFinished();

private slots:
    void onTestRunStarted();
    void onTestResultReady(const Autotest::TestResult &result);
    void onTestRunFinished();
    void onReportSummary(const QString &id, const QHash<Autotest::ResultType, int> &summary);
    void onReportDuration(int duration);

private:
    void connectSignals();

    QList<Autotest::TestResult> m_results;
    QHash<Autotest::ResultType, int> m_summary;
    int m_durationMs = -1;
    bool m_running = false;
    bool m_buildFailed = false;
    bool m_signalsConnected = false;
};

} // namespace Autotest::Internal
