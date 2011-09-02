/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QTTESTPLUGIN_H
#define QTTESTPLUGIN_H

#include "testsuite.h"

#include <projectexplorer/project.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <extensionsystem/iplugin.h>

class TestContextMenu;
class TestResultsWindow;
class TestOutputWindow;

namespace QtTest {
namespace Internal {

class TestNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
public:
    TestNavigationWidgetFactory() {}
    ~TestNavigationWidgetFactory() {}
    Core::NavigationView createWidget();
    QString displayName() const;
    QString id() const { return "QtTestNavigationWidget"; }
    int priority() const { return 750; }
};

class QtTestPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    QtTestPlugin();
    ~QtTestPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    void extensionsInitialized();

public slots:
    void onDefectSelected(TestCaseRec rec);
    void onStartupProjectChanged(ProjectExplorer::Project *project);
    void onProjectRemoved(ProjectExplorer::Project *project);
    void onAllTasksFinished(const QString &);

    void testRun();
    void testRunAsManual();
    void testDebug();
    void testRunSingle();
    void stopTesting();
    void retryTests(const QStringList &tests);
    void insertTestFunction();

protected:
    void runSelectedTests(bool singleTest, bool forceManual);
    ProjectExplorer::Project *startupProject();

private:
    TestOutputWindow *m_messageOutputWindow;
    TestResultsWindow *m_testResultsWindow;
    TestCollection m_testCollection;
    TestContextMenu *m_contextMenu;
    bool m_forceManual;
};

} // namespace Internal
} // namespace QtTest

#endif //QTTESTPLUGIN_H
