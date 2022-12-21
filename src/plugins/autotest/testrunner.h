// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "autotest_global.h"
#include "testresult.h"

#include <QDialog>
#include <QFutureWatcher>
#include <QObject>
#include <QQueue>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }
namespace Utils { class QtcProcess; }

namespace Autotest {

enum class TestRunMode;
class ITestConfiguration;
class TestOutputReader;

namespace Internal {

class AUTOTESTSHARED_EXPORT TestRunner final : public QObject
{
    Q_OBJECT

public:
    TestRunner();
    ~TestRunner() final;

    enum CancelReason { UserCanceled, Timeout, KitChanged };

    static TestRunner* instance();

    void setSelectedTests(const QList<ITestConfiguration *> &selected);
    void runTest(TestRunMode mode, const ITestTreeItem *item);
    bool isTestRunning() const { return m_executingTests; }

    void prepareToRunTests(TestRunMode mode);

signals:
    void testRunStarted();
    void testRunFinished();
    void requestStopTestRun();
    void testResultReady(const TestResultPtr &result);
    void hadDisabledTests(int disabled);
    void reportSummary(const QString &id, const QHash<ResultType, int> &summary);

private:
    void buildProject(ProjectExplorer::Project *project);
    void buildFinished(bool success);
    void onBuildQueueFinished(bool success);
    void onFinished();

    int precheckTestConfigurations();
    bool currentConfigValid();
    void setUpProcess();
    void setUpProcessEnv();
    void scheduleNext();
    void cancelCurrent(CancelReason reason);
    void onProcessDone();
    void resetInternalPointers();

    void runTests();
    void debugTests();
    void runOrDebugTests();
    void reportResult(ResultType type, const QString &description);
    bool postponeTestRunWithEmptyExecutable(ProjectExplorer::Project *project);
    void onBuildSystemUpdated();

    QFutureWatcher<TestResultPtr> m_futureWatcher;
    QFutureInterface<TestResultPtr> *m_fakeFutureInterface = nullptr;
    QQueue<ITestConfiguration *> m_selectedTests;
    bool m_executingTests = false;
    bool m_canceled = false;
    ITestConfiguration *m_currentConfig = nullptr;
    Utils::QtcProcess *m_currentProcess = nullptr;
    TestOutputReader *m_currentOutputReader = nullptr;
    TestRunMode m_runMode = TestRunMode::None;

    // temporarily used if building before running is necessary
    QMetaObject::Connection m_buildConnect;
    // temporarily used when debugging
    QMetaObject::Connection m_stopDebugConnect;
    QMetaObject::Connection m_finishDebugConnect;
    // temporarily used for handling of switching the current target
    QMetaObject::Connection m_targetConnect;
    QTimer m_cancelTimer;
    bool m_skipTargetsCheck = false;
};

class RunConfigurationSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RunConfigurationSelectionDialog(const QString &buildTargetKey, QWidget *parent = nullptr);
    QString displayName() const;
    QString executable() const;
    bool rememberChoice() const;
private:
    void populate();
    void updateLabels();
    QLabel *m_details;
    QLabel *m_executable;
    QLabel *m_arguments;
    QLabel *m_workingDir;
    QComboBox *m_rcCombo;
    QCheckBox *m_rememberCB;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace Autotest
