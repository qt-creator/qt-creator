/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
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

#include "clangstaticanalyzerruncontrolfactory.h"

#include "clangstaticanalyzerconstants.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojects.h>

#include <projectexplorer/buildconfiguration.h>
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
                                                  Core::Id runMode) const
{
    if (runMode != Constants::CLANGSTATICANALYZER_RUN_MODE)
        return false;

    Target *target = runConfiguration->target();
    QTC_ASSERT(target, return false);
    Kit *kit = target->kit();
    QTC_ASSERT(kit, return false);
    ToolChain *toolChain = ToolChainKitInformation::toolChain(kit);
    return toolChain && (toolChain->type() == QLatin1String("clang")
            || toolChain->type() == QLatin1String("gcc")
            || toolChain->type() == QLatin1String("mingw")
            || toolChain->type() == QLatin1String("msvc"));
}

RunControl *ClangStaticAnalyzerRunControlFactory::create(RunConfiguration *runConfiguration,
                                                         Core::Id runMode,
                                                         QString *errorMessage)
{
    using namespace CppTools;
    const ProjectInfo projectInfoBeforeBuild = m_tool->projectInfoBeforeBuild();
    QTC_ASSERT(projectInfoBeforeBuild.isValid(), return 0);

    QTC_ASSERT(runConfiguration, return 0);
    Target * const target = runConfiguration->target();
    QTC_ASSERT(target, return 0);
    Project * const project = target->project();
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
    BuildConfiguration * const buildConfiguration = target->activeBuildConfiguration();
    QTC_ASSERT(buildConfiguration, return 0);
    sp.environment = buildConfiguration->environment();

    return AnalyzerManager::createRunControl(sp, runConfiguration);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
