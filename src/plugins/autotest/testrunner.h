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

#include "testconfiguration.h"
#include "testresult.h"

#include <QFutureWatcher>
#include <QObject>
#include <QProcess>

namespace ProjectExplorer {
class Project;
}

namespace Autotest {
namespace Internal {

class TestRunner : public QObject
{
    Q_OBJECT

public:
    enum Mode
    {
        Run,
        RunWithoutDeploy,
        Debug,
        DebugWithoutDeploy
    };

    static TestRunner* instance();
    ~TestRunner();

    void setSelectedTests(const QList<TestConfiguration *> &selected);
    bool isTestRunning() const { return m_executingTests; }

    void prepareToRunTests(Mode mode);

signals:
    void testRunStarted();
    void testRunFinished();
    void requestStopTestRun();
    void testResultReady(const TestResultPtr &result);

private:
    void buildProject(ProjectExplorer::Project *project);
    void buildFinished(bool success);
    void onFinished();

    void runTests();
    void debugTests();
    void runOrDebugTests();
    explicit TestRunner(QObject *parent = 0);

    QFutureWatcher<TestResultPtr> m_futureWatcher;
    QList<TestConfiguration *> m_selectedTests;
    bool m_executingTests;
    Mode m_runMode = Run;

    // temporarily used if building before running is necessary
    QMetaObject::Connection m_buildConnect;
};

} // namespace Internal
} // namespace Autotest
