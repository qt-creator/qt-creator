/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "testconfiguration.h"
#include "testresult.h"

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
    static TestRunner* instance();
    ~TestRunner();

    void setSelectedTests(const QList<TestConfiguration *> &selected);
    bool isTestRunning() const { return m_executingTests; }

signals:
    void testRunStarted();
    void testRunFinished();
    void testResultCreated(TestResult *testResult);
    void requestStopTestRun();

public slots:
    void prepareToRunTests();

private slots:
    void buildProject(ProjectExplorer::Project *project);
    void buildFinished(bool success);
    void onFinished();

private:
    void runTests();
    explicit TestRunner(QObject *parent = 0);

    QList<TestConfiguration *> m_selectedTests;
    bool m_executingTests;

    // temporarily used if building before running is necessary
    QMetaObject::Connection m_buildConnect;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTRUNNER_H
