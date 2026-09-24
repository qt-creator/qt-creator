// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaltestrun.h"

#include "autotestplugin.h"
#include "testresultspane.h"
#include "testrunner.h"
#include "testtreemodel.h"

#include <utils/qtcassert.h>

#include <QThread>

namespace Autotest {

using namespace Internal;

// Only an item a test starts with takes the test's own messages as children,
// so that they end up as a list under it instead of nesting ever deeper.
static ResultHooks::DirectParentHook directParentHook()
{
    return [](const TestResult &result, const TestResult &, bool *) -> bool {
        return result.result() == ResultType::TestStart;
    };
}

ExternalTestRun::ExternalTestRun(const QString &suiteName,
                                 const std::function<void()> &cancelHandler)
    : m_suiteName(suiteName)
{
    QTC_ASSERT(QThread::isMainThread(), return);
    TestRunner *runner = TestRunner::instance();
    QTC_ASSERT(runner, return);
    if (runner->isTestRunning())
        return;
    m_running = true;
    runner->m_externalRunning = true;
    if (cancelHandler) {
        runner->m_externalCancelable = true;
        m_cancelConnection = QObject::connect(runner, &TestRunner::requestStopTestRun,
                                              runner, cancelHandler);
    }
    TestResultsPane::instance()->clearContents();
    TestTreeModel::instance()->clearFailedMarks();
    emit runner->testRunStarted();
    updateMenuItemsEnabledState();
}

ExternalTestRun::~ExternalTestRun()
{
    if (!m_running)
        return;
    TestRunner *runner = TestRunner::instance();
    QTC_ASSERT(runner, return);
    QObject::disconnect(m_cancelConnection);
    // queued, so results reported from other threads before this land first
    QMetaObject::invokeMethod(runner, [runner] {
        runner->m_externalCancelable = false;
        runner->m_externalRunning = false;
        emit runner->testRunFinished();
        updateMenuItemsEnabledState();
    }, Qt::QueuedConnection);
}

void ExternalTestRun::reportResult(const QString &testName, ResultType type,
                                   const QString &message, const Utils::FilePath &file, int line,
                                   int durationMs)
{
    if (!m_running)
        return;
    TestRunner *runner = TestRunner::instance();
    QTC_ASSERT(runner, return);
    TestResult result(m_suiteName, testName, {{}, {}, {}, directParentHook()});
    result.setResult(type);
    result.setDescription(message.isEmpty() ? testName : message);
    result.setFileName(file);
    result.setLine(line);
    if (durationMs >= 0)
        result.setDuration(QString::number(durationMs));
    emit runner->testResultReady(result);
}

void ExternalTestRun::reportOutput(const QString &text)
{
    if (!m_running)
        return;
    TestRunner *runner = TestRunner::instance();
    QTC_ASSERT(runner, return);
    // The pane takes one line at a time, an extension appends whole blocks.
    QStringList lines = text.split('\n');
    if (!lines.isEmpty() && lines.constLast().isEmpty()) // A trailing newline is no line.
        lines.removeLast();
    QMetaObject::invokeMethod(runner, [lines] {
        for (const QString &line : lines)
            TestResultsPane::instance()->addOutputLine(line.toUtf8(), OutputChannel::StdOut);
    });
}

} // namespace Autotest
