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

#include "clangstaticanalyzerpreconfiguredsessiontests.h"

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzertool.h"
#include "clangstaticanalyzerutils.h"

#include <cpptools/projectinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QSignalSpy>
#include <QTimer>
#include <QtTest>
#include <QVariant>

#include <functional>

using namespace CppTools;
using namespace ProjectExplorer;

static bool processEventsUntil(const std::function<bool()> condition, int timeOutInMs = 30000)
{
    QTime t;
    t.start();

    forever {
        if (t.elapsed() > timeOutInMs)
            return false;

        if (condition())
            return true;

        QCoreApplication::processEvents();
    }
}

class WaitForParsedProjects : public QObject
{
public:
    WaitForParsedProjects(const QStringList &projects)
        : m_projectsToWaitFor(projects)
    {
        connect(SessionManager::instance(),
                &ProjectExplorer::SessionManager::projectFinishedParsing,
                this, &WaitForParsedProjects::onProjectFinishedParsing);
    }

    void onProjectFinishedParsing(ProjectExplorer::Project *project)
    {
        m_projectsToWaitFor.removeOne(project->projectFilePath().toString());
    }

    bool wait()
    {
        return processEventsUntil([this]() {
            return m_projectsToWaitFor.isEmpty();
        });
    }

private:
    QStringList m_projectsToWaitFor;
};

namespace ClangStaticAnalyzer {
namespace Internal {

void ClangStaticAnalyzerPreconfiguredSessionTests::initTestCase()
{
    const QString preconfiguredSessionName = QLatin1String("ClangStaticAnalyzerPreconfiguredSession");
    if (!SessionManager::sessions().contains(preconfiguredSessionName))
        QSKIP("Manually preconfigured session 'ClangStaticAnalyzerPreconfiguredSession' needed.");

    if (SessionManager::activeSession() == preconfiguredSessionName)
        QSKIP("Session must not be already active.");

    // Load session
    const QStringList projects = SessionManager::projectsForSessionName(preconfiguredSessionName);
    WaitForParsedProjects waitForParsedProjects(projects);
    QVERIFY(SessionManager::loadSession(preconfiguredSessionName));
    QVERIFY(waitForParsedProjects.wait());
}

void ClangStaticAnalyzerPreconfiguredSessionTests::testPreconfiguredSession()
{
    QFETCH(Project *, project);
    QFETCH(Target *, target);

    QVERIFY(switchToProjectAndTarget(project, target));

    ClangStaticAnalyzerTool::instance()->startTool();
    QSignalSpy waitUntilAnalyzerFinished(ClangStaticAnalyzerTool::instance(), SIGNAL(finished(bool)));
    QVERIFY(waitUntilAnalyzerFinished.wait(30000));
    const QList<QVariant> arguments = waitUntilAnalyzerFinished.takeFirst();
    const bool analyzerFinishedSuccessfully = arguments.first().toBool();
    QVERIFY(analyzerFinishedSuccessfully);
    QCOMPARE(ClangStaticAnalyzerTool::instance()->diagnostics().count(), 0);
}

static QList<Project *> validProjects(const QList<Project *> projectsOfSession)
{
    QList<Project *> sortedProjects = projectsOfSession;
    Utils::sort(sortedProjects, [](Project *lhs, Project *rhs){
        return lhs->displayName() < rhs->displayName();
    });

    const auto isValidProject = [](Project *project) {
        const QList <Target *> targets = project->targets();
        if (targets.isEmpty()) {
            qWarning("Skipping project \"%s\" since it has no targets.",
                     qPrintable(project->projectFilePath().fileName()));
        }
        return !targets.isEmpty();
    };

    return Utils::filtered(sortedProjects, isValidProject);
}

static QList<Target *> validTargets(Project *project)
{
    QList<Target *> sortedTargets = project->targets();
    Utils::sort(sortedTargets, [](Target *lhs, Target *rhs){
        return lhs->displayName() < rhs->displayName();
    });

    const QString projectFileName = project->projectFilePath().fileName();
    const auto isValidTarget = [projectFileName](Target *target) {
        Kit *kit = target->kit();
        if (!kit || !kit->isValid()) {
            qWarning("Project \"%s\": Skipping target \"%s\" since it has no (valid) kits.",
                     qPrintable(projectFileName),
                     qPrintable(target->displayName()));
            return false;
        }

        const ToolChain * const toolchain = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        QTC_ASSERT(toolchain, return false);
        bool hasClangExecutable;
        clangExecutableFromSettings(&hasClangExecutable);
        if (!hasClangExecutable) {
            qWarning("Project \"%s\": Skipping target \"%s\" since no suitable clang was found for the toolchain.",
                     qPrintable(projectFileName),
                     qPrintable(target->displayName()));
            return false;
        }

        return true;
    };

    return Utils::filtered(sortedTargets, isValidTarget);
}

static QByteArray dataTagName(Project *project, Target *target)
{
    const QString projectFileName = project->projectFilePath().fileName();
    const QString dataTagAsString = projectFileName + " -- " + target->displayName();
    return dataTagAsString.toUtf8();
}

void ClangStaticAnalyzerPreconfiguredSessionTests::testPreconfiguredSession_data()
{
    QTest::addColumn<Project *>("project");
    QTest::addColumn<Target *>("target");

    bool hasAddedTestData = false;

    foreach (Project *project, validProjects(SessionManager::projects())) {
        foreach (Target *target, validTargets(project)) {
            hasAddedTestData = true;
            QTest::newRow(dataTagName(project, target)) << project << target;
        }
    }

    if (!hasAddedTestData)
        QSKIP("Session has no valid projects/targets to test.");
}

bool ClangStaticAnalyzerPreconfiguredSessionTests::switchToProjectAndTarget(Project *project,
                                                                            Target *target)
{
    Project * const activeProject = SessionManager::startupProject();
    if (project == activeProject && target == activeProject->activeTarget())
        return true; // OK, desired project/target already active.

    if (project != activeProject)
        SessionManager::setStartupProject(project);

    if (target != project->activeTarget()) {
        QSignalSpy spyFinishedParsing(ProjectExplorer::SessionManager::instance(),
                                      &ProjectExplorer::SessionManager::projectFinishedParsing);
        SessionManager::setActiveTarget(project, target, ProjectExplorer::SetActive::NoCascade);
        QTC_ASSERT(spyFinishedParsing.wait(30000), return false);

        const QVariant projectArgument = spyFinishedParsing.takeFirst().takeFirst();
        QTC_ASSERT(projectArgument.canConvert<ProjectExplorer::Project *>(), return false);

        return projectArgument.value<ProjectExplorer::Project *>() == project;
    }

    return true;
}

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin
