// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolspreconfiguredsessiontests.h"

#include "clangtool.h"
#include "clangtoolsdiagnostic.h"

#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/projectinfo.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QSignalSpy>
#include <QElapsedTimer>
#include <QtTest>
#include <QVariant>

#include <functional>

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

static bool processEventsUntil(const std::function<bool()> condition, int timeOutInMs = 30000)
{
    QElapsedTimer t;
    t.start();

    while (true) {
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
    WaitForParsedProjects(const FilePaths &projects)
        : m_projectsToWaitFor(projects)
    {
        connect(ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::projectFinishedParsing,
                this, &WaitForParsedProjects::onProjectFinishedParsing);
    }

    void onProjectFinishedParsing(ProjectExplorer::Project *project)
    {
        m_projectsToWaitFor.removeOne(project->projectFilePath());
    }

    bool wait()
    {
        return processEventsUntil([this] { return m_projectsToWaitFor.isEmpty(); });
    }

private:
    FilePaths m_projectsToWaitFor;
};

namespace ClangTools {
namespace Internal {

void PreconfiguredSessionTests::initTestCase()
{
    const QString preconfiguredSessionName = QLatin1String("ClangToolsTest");
    if (!SessionManager::sessions().contains(preconfiguredSessionName))
        QSKIP("Manually preconfigured session 'ClangToolsTest' needed.");

    if (SessionManager::activeSession() == preconfiguredSessionName)
        QSKIP("Session must not be already active.");

    // Load session
    const FilePaths projects = ProjectManager::projectsForSessionName(preconfiguredSessionName);
    WaitForParsedProjects waitForParsedProjects(projects);
    QVERIFY(SessionManager::loadSession(preconfiguredSessionName));
    QVERIFY(waitForParsedProjects.wait());
}

void PreconfiguredSessionTests::testPreconfiguredSession()
{
    QFETCH(Project *, project);
    QFETCH(Target *, target);

    QVERIFY(switchToProjectAndTarget(project, target));

    for (ClangTool * const tool : {ClangTidyTool::instance(), ClazyTool::instance()}) {
        tool->startTool(ClangTool::FileSelectionType::AllFiles);
        QSignalSpy waitUntilAnalyzerFinished(tool, &ClangTool::finished);
        QVERIFY(waitUntilAnalyzerFinished.wait(30000));
        const QList<QVariant> arguments = waitUntilAnalyzerFinished.takeFirst();
        const bool analyzerFinishedSuccessfully = arguments.first().toBool();
        QVERIFY(analyzerFinishedSuccessfully);
        QCOMPARE(tool->diagnostics().count(), 0);
    }
}

static const QList<Project *> validProjects(const QList<Project *> projectsOfSession)
{
    const QList<Project *> sortedProjects = Utils::sorted(projectsOfSession,
                [](Project *lhs, Project *rhs) {  return lhs->displayName() < rhs->displayName(); }
    );

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
    const QList<Target *> sortedTargets = Utils::sorted(project->targets(),
                [](Target *lhs, Target *rhs) { return lhs->displayName() < rhs->displayName(); }
    );

    const QString projectFileName = project->projectFilePath().fileName();
    const auto isValidTarget = [projectFileName](Target *target) {
        Kit *kit = target->kit();
        if (!kit || !kit->isValid()) {
            qWarning("Project \"%s\": Skipping target \"%s\" since it has no (valid) kits.",
                     qPrintable(projectFileName),
                     qPrintable(target->displayName()));
            return false;
        }

        const ToolChain * const toolchain = ToolChainKitAspect::cxxToolChain(kit);
        QTC_ASSERT(toolchain, return false);

        if (Core::ICore::clangExecutable(CLANG_BINDIR).isEmpty()) {
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

void PreconfiguredSessionTests::testPreconfiguredSession_data()
{
    QTest::addColumn<Project *>("project");
    QTest::addColumn<Target *>("target");

    bool hasAddedTestData = false;

    for (Project *project : validProjects(ProjectManager::projects())) {
        for (Target *target : validTargets(project)) {
            hasAddedTestData = true;
            QTest::newRow(dataTagName(project, target)) << project << target;
        }
    }

    if (!hasAddedTestData)
        QSKIP("Session has no valid projects/targets to test.");
}

bool PreconfiguredSessionTests::switchToProjectAndTarget(Project *project,
                                                                            Target *target)
{
    Project * const activeProject = ProjectManager::startupProject();
    if (project == activeProject && target == activeProject->activeTarget())
        return true; // OK, desired project/target already active.

    if (project != activeProject)
        ProjectManager::setStartupProject(project);

    if (target != project->activeTarget()) {
        QSignalSpy spyFinishedParsing(ProjectExplorer::ProjectManager::instance(),
                                      &ProjectExplorer::ProjectManager::projectFinishedParsing);
        project->setActiveTarget(target, ProjectExplorer::SetActive::NoCascade);
        QTC_ASSERT(spyFinishedParsing.wait(30000), return false);

        const QVariant projectArgument = spyFinishedParsing.takeFirst().constFirst();
        QTC_ASSERT(projectArgument.canConvert<ProjectExplorer::Project *>(), return false);

        return projectArgument.value<ProjectExplorer::Project *>() == project;
    }

    return true;
}

} // namespace Internal
} // namespace ClangTools
