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

#include <cpptools/cppmodelmanager.h>
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

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerPreconfiguredSessionTests::ClangStaticAnalyzerPreconfiguredSessionTests(
        ClangStaticAnalyzerTool *analyzerTool,
        QObject *parent)
    : QObject(parent)
    , m_sessionManager(*ProjectExplorer::SessionManager::instance())
    , m_analyzerTool(*analyzerTool)
{
}

void ClangStaticAnalyzerPreconfiguredSessionTests::initTestCase()
{
    const QString preconfiguredSessionName = QLatin1String("ClangStaticAnalyzerPreconfiguredSession");
    if (!m_sessionManager.sessions().contains(preconfiguredSessionName))
        QSKIP("Manually preconfigured session 'ClangStaticAnalyzerPreconfiguredSession' needed.");

    // Load session
    if (m_sessionManager.activeSession() != preconfiguredSessionName)
        QVERIFY(m_sessionManager.loadSession(preconfiguredSessionName));

    // Wait until all projects are loaded.
    const int sessionManagerProjects = m_sessionManager.projects().size();
    const auto allProjectsLoaded = [sessionManagerProjects]() {
        return CppModelManager::instance()->projectInfos().size() == sessionManagerProjects;
    };
    QVERIFY(processEventsUntil(allProjectsLoaded));
}

void ClangStaticAnalyzerPreconfiguredSessionTests::testPreconfiguredSession()
{
    QFETCH(Project *, project);
    QFETCH(Target *, target);

    QVERIFY(switchToProjectAndTarget(project, target));

    m_analyzerTool.startTool();
    QSignalSpy waitUntilAnalyzerFinished(&m_analyzerTool, SIGNAL(finished(bool)));
    QVERIFY(waitUntilAnalyzerFinished.wait(30000));
    const QList<QVariant> arguments = waitUntilAnalyzerFinished.takeFirst();
    QVERIFY(arguments.first().toBool());
    QCOMPARE(m_analyzerTool.diagnostics().count(), 0);
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

        const ToolChain * const toolchain = ToolChainKitInformation::toolChain(kit, ToolChain::Language::Cxx);
        QTC_ASSERT(toolchain, return false);
        bool hasClangExecutable;
        clangExecutableFromSettings(toolchain->typeId(), &hasClangExecutable);
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

    foreach (Project *project, validProjects(m_sessionManager.projects())) {
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
    Project * const activeProject = m_sessionManager.startupProject();
    if (project == activeProject && target == activeProject->activeTarget())
        return true; // OK, desired project/target already active.

    QSignalSpy waitUntilProjectUpdated(CppModelManager::instance(),
                                       &CppModelManager::projectPartsUpdated);

    if (project != activeProject)
        m_sessionManager.setStartupProject(project);
    m_sessionManager.setActiveTarget(project, target, ProjectExplorer::SetActive::NoCascade);

    const bool waitResult = waitUntilProjectUpdated.wait(30000);
    if (!waitResult) {
        qWarning() << "waitUntilProjectUpdated() failed";
        return false;
    }

    return true;
}

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin
