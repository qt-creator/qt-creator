/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerruncontrolfactory.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojects.h>

#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerRunControlFactory::ClangStaticAnalyzerRunControlFactory(
        ClangStaticAnalyzerTool *tool,
        QObject *parent)
    : IRunControlFactory(parent)
    , m_tool(tool)
{
    QTC_CHECK(m_tool);
}

bool ClangStaticAnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration,
                                                  RunMode runMode) const
{
    if (runMode != ClangStaticAnalyzerMode)
        return false;

    Target *target = runConfiguration->target();
    QTC_ASSERT(target, return false);
    Kit *kit = target->kit();
    QTC_ASSERT(kit, return false);
    ToolChain *toolChain = ToolChainKitInformation::toolChain(kit);
    QTC_ASSERT(toolChain, return false);
    return toolChain->type() == QLatin1String("clang") || toolChain->type() == QLatin1String("gcc");
}

RunControl *ClangStaticAnalyzerRunControlFactory::create(RunConfiguration *runConfiguration,
                                                         RunMode runMode,
                                                         QString *errorMessage)
{
    Q_UNUSED(runMode);

    using namespace CppTools;
    const ProjectInfo projectInfoBeforeBuild = m_tool->projectInfoBeforeBuild();
    QTC_ASSERT(projectInfoBeforeBuild.isValid(), return 0);

    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, return 0);
    const ProjectInfo projectInfoAfterBuild = CppModelManager::instance()->projectInfo(project);

    if (projectInfoAfterBuild.configurationOrFilesChanged(projectInfoBeforeBuild)) {
        // If it's more than a release/debug build configuration change, e.g.
        // a version control checkout, files might be not valid C++ anymore
        // or even gone, so better stop here.

        m_tool->resetCursorAndProjectInfoBeforeBuild();
        if (errorMessage) {
            *errorMessage = tr(
                "The project configuration changed since the start of the Clang Static Analyzer. "
                "Please re-run with current configuration.");
        }
        return 0;
    }

    AnalyzerStartParameters sp;
    sp.runMode = runMode;
    sp.startMode = StartLocal;
    return AnalyzerManager::createRunControl(sp, runConfiguration);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
