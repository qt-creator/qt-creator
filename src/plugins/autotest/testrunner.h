/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "autotest_global.h"

#include "testconfiguration.h"
#include "testresult.h"

#include <QDialog>
#include <QFutureWatcher>
#include <QObject>
#include <QQueue>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QProcess;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
}

namespace Autotest {

enum class TestRunMode;

namespace Internal {

class AUTOTESTSHARED_EXPORT TestRunner final : public QObject
{
    Q_OBJECT

public:
    TestRunner();
    ~TestRunner() final;

    enum CancelReason { UserCanceled, Timeout, KitChanged };

    static TestRunner* instance();

    void setSelectedTests(const QList<TestConfiguration *> &selected);
    void runTest(TestRunMode mode, const TestTreeItem *item);
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
    void scheduleNext();
    void cancelCurrent(CancelReason reason);
    void onProcessFinished();
    void resetInternalPointers();

    void runTests();
    void debugTests();
    void runOrDebugTests();
    void reportResult(ResultType type, const QString &description);
    bool postponeTestRunWithEmptyExecutable(ProjectExplorer::Project *project);
    void onBuildSystemUpdated();

    QFutureWatcher<TestResultPtr> m_futureWatcher;
    QFutureInterface<TestResultPtr> *m_fakeFutureInterface = nullptr;
    QQueue<TestConfiguration *> m_selectedTests;
    bool m_executingTests = false;
    bool m_canceled = false;
    TestConfiguration *m_currentConfig = nullptr;
    QProcess *m_currentProcess = nullptr;
    TestOutputReader *m_currentOutputReader = nullptr;
    TestRunMode m_runMode = TestRunMode::None;

    // temporarily used if building before running is necessary
    QMetaObject::Connection m_buildConnect;
    // temporarily used when debugging
    QMetaObject::Connection m_stopDebugConnect;
    QMetaObject::Connection m_finishDebugConnect;
    // temporarily used for handling of switching the current target
    QMetaObject::Connection m_targetConnect;
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
