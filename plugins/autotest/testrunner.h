/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
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

public slots:
    void runTests();
    void stopTestRun();

private slots:
    void buildProject(ProjectExplorer::Project *project);
    void buildFinished(bool success);
    void onFinished();

private:
    explicit TestRunner(QObject *parent = 0);

    QList<TestConfiguration *> m_selectedTests;
    bool m_building;
    bool m_buildSucceeded;
    bool m_executingTests;

};

} // namespace Internal
} // namespace Autotest

#endif // TESTRUNNER_H
