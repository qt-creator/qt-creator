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

#include "clangstaticanalyzerruncontrol.h"

#include "clangtoolslogfilereader.h"
#include "clangstaticanalyzerrunner.h"
#include "clangtoolssettings.h"
#include "clangstaticanalyzertool.h"
#include "clangtoolsutils.h"

#include <debugger/analyzer/analyzerconstants.h>

#include <clangcodemodel/clangutils.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojectfile.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>
#include <utils/qtcprocess.h>

#include <QAction>
#include <QLoggingCategory>

using namespace ProjectExplorer;

namespace ClangTools {
namespace Internal {

class ProjectBuilder : public RunWorker, public BaseProjectBuilder
{
public:
    ProjectBuilder(RunControl *runControl, Project *project)
        : RunWorker(runControl), m_project(project)
    {
        setDisplayName("ProjectBuilder");
    }

    bool success() const override { return m_success; }

private:
    void start() final
    {
        Target *target = m_project->activeTarget();
        QTC_ASSERT(target, reportFailure(); return);

        BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
        if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
            buildType = buildConfig->buildType();

        if (buildType == BuildConfiguration::Release) {
            const QString wrongMode = ClangStaticAnalyzerTool::tr("Release");
            const QString toolName = ClangStaticAnalyzerTool::tr("Clang Static Analyzer");
            const QString title = ClangStaticAnalyzerTool::tr("Run %1 in %2 Mode?").arg(toolName)
                    .arg(wrongMode);
            const QString message = ClangStaticAnalyzerTool::tr(
                        "<html><head/><body>"
                        "<p>You are trying to run the tool \"%1\" on an application in %2 mode. The tool is "
                        "designed to be used in Debug mode since enabled assertions can reduce the number of "
                        "false positives.</p>"
                        "<p>Do you want to continue and run the tool in %2 mode?</p>"
                        "</body></html>")
                    .arg(toolName).arg(wrongMode);
            if (Utils::CheckableMessageBox::doNotAskAgainQuestion(Core::ICore::mainWindow(),
                                                                  title, message, Core::ICore::settings(),
                                                                  "ClangStaticAnalyzerCorrectModeWarning") != QDialogButtonBox::Yes)
            {
                reportFailure();
                return;
            }
        }

        connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
                this, &ProjectBuilder::onBuildFinished, Qt::QueuedConnection);

        ProjectExplorerPlugin::buildProject(m_project);
     }

     void onBuildFinished(bool success)
     {
         disconnect(BuildManager::instance(), &BuildManager::buildQueueFinished,
                    this, &ProjectBuilder::onBuildFinished);
         m_success = success;
         reportDone();
     }

private:
     QPointer<Project> m_project;
     bool m_success = false;
};

ClangStaticAnalyzerRunControl::ClangStaticAnalyzerRunControl(RunControl *runControl, Target *target)
    : ClangToolRunControl(runControl, target)
{
    setDisplayName("ClangStaticAnalyzerRunner");

    auto *projectBuilder = new ProjectBuilder(runControl, target->project());
    addStartDependency(projectBuilder);
    m_projectBuilder = projectBuilder;

    init();
}

ClangToolRunner *ClangStaticAnalyzerRunControl::createRunner()
{
    QTC_ASSERT(!m_clangLogFileDir.isEmpty(), return 0);

    auto runner = new ClangStaticAnalyzerRunner(m_clangExecutable,
                                                m_clangLogFileDir,
                                                m_environment,
                                                this);
    connect(runner, &ClangStaticAnalyzerRunner::finishedWithSuccess,
            this, &ClangStaticAnalyzerRunControl::onRunnerFinishedWithSuccess);
    connect(runner, &ClangStaticAnalyzerRunner::finishedWithFailure,
            this, &ClangStaticAnalyzerRunControl::onRunnerFinishedWithFailure);
    return runner;
}

ClangTool *ClangStaticAnalyzerRunControl::tool()
{
    return ClangStaticAnalyzerTool::instance();
}

} // namespace Internal
} // namespace ClangTools
