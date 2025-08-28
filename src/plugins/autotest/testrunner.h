// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "autotest_global.h"

#include "autotestconstants.h"

#include <solutions/tasking/tasktreerunner.h>

#include <QList>
#include <QTimer>

namespace ProjectExplorer { class Project; }

namespace Autotest {

class ITestConfiguration;
class ITestTreeItem;
class TestResult;
enum class ResultType;

namespace Internal {

class AUTOTESTSHARED_EXPORT TestRunner final : public QObject
{
    Q_OBJECT

public:
    TestRunner();
    ~TestRunner() final;

    enum CancelReason { UserCanceled, Timeout, KitChanged };

    static TestRunner* instance();

    void runTests(TestRunMode mode, const QList<ITestConfiguration *> &selectedTests);
    void runTest(TestRunMode mode, const ITestTreeItem *item);
    bool isTestRunning() const
    {
        return m_buildConnect || m_stopDebugConnect || m_taskTreeRunner.isRunning();
    }

signals:
    void testRunStarted();
    void testRunFinished();
    void requestStopTestRun();
    void testResultReady(const TestResult &result);
    void hadDisabledTests(int disabled);
    void reportSummary(const QString &id, const QHash<ResultType, int> &summary);
    void reportDuration(int duration);

private:
    void buildProject(ProjectExplorer::Project *project);
    void buildFinished(bool success);
    void onBuildQueueFinished(bool success);
    void onFinished();

    int precheckTestConfigurations();
    void cancelCurrent(CancelReason reason);

    void runTestsHelper();
    void debugTests();
    void runOrDebugTests();
    void reportResult(ResultType type, const QString &description);
    bool postponeTestRunWithEmptyExecutable(ProjectExplorer::Project *project);
    void onBuildSystemUpdated();

    Tasking::TaskTreeRunner m_taskTreeRunner;

    QList<ITestConfiguration *> m_selectedTests;
    TestRunMode m_runMode = TestRunMode::None;

    // temporarily used if building before running is necessary
    QMetaObject::Connection m_buildConnect;
    // temporarily used when debugging
    QMetaObject::Connection m_stopDebugConnect;
    // temporarily used for handling of switching the current target
    QMetaObject::Connection m_targetConnect;
    QTimer m_cancelTimer;
    bool m_skipTargetsCheck = false;
};

} // namespace Internal
} // namespace Autotest
