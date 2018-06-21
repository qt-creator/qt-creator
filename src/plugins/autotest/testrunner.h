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
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QProcess;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
}

namespace Autotest {
namespace Internal {

class AUTOTESTSHARED_EXPORT TestRunner : public QObject
{
    Q_OBJECT

public:
    enum CancelReason { UserCanceled, Timeout };

    static TestRunner* instance();
    ~TestRunner();

    void setSelectedTests(const QList<TestConfiguration *> &selected);
    void runTest(TestRunMode mode, const TestTreeItem *item);
    bool isTestRunning() const { return m_executingTests; }

    void prepareToRunTests(TestRunMode mode);

signals:
    void testRunStarted();
    void testRunFinished();
    void requestStopTestRun();
    void testResultReady(const TestResultPtr &result);

private:
    void buildProject(ProjectExplorer::Project *project);
    void buildFinished(bool success);
    void onFinished();

    int precheckTestConfigurations();
    void scheduleNext();
    void cancelCurrent(CancelReason reason);
    void onProcessFinished();
    void resetInternalPointers();

    void runTests();
    void debugTests();
    void runOrDebugTests();
    explicit TestRunner(QObject *parent = nullptr);

    QFutureWatcher<TestResultPtr> m_futureWatcher;
    QFutureInterface<TestResultPtr> *m_fakeFutureInterface = nullptr;
    QQueue<TestConfiguration *> m_selectedTests;
    bool m_executingTests = false;
    TestConfiguration *m_currentConfig = nullptr;
    QProcess *m_currentProcess = nullptr;
    TestOutputReader *m_currentOutputReader = nullptr;
    TestRunMode m_runMode = TestRunMode::Run;

    // temporarily used if building before running is necessary
    QMetaObject::Connection m_buildConnect;
};

class RunConfigurationSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RunConfigurationSelectionDialog(const QString &testsInfo, QWidget *parent = nullptr);
    QString displayName() const;
    QString executable() const;
private:
    void populate();
    void updateLabels();
    QLabel *m_details;
    QLabel *m_executable;
    QLabel *m_arguments;
    QLabel *m_workingDir;
    QComboBox *m_rcCombo;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace Autotest
