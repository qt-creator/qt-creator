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

#include "clangstaticanalyzerruncontrolfactory.h"

#include "clangstaticanalyzerconstants.h"

#include <coreplugin/icontext.h>

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/qtcassert.h>

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

    Project *project = runConfiguration->target()->project();
    QTC_ASSERT(project, return false);
    const Core::Context context = project->projectLanguages();
    if (!context.contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID))
        return false;

    Target *target = runConfiguration->target();
    QTC_ASSERT(target, return false);
    Kit *kit = target->kit();
    QTC_ASSERT(kit, return false);
    ToolChain *toolChain = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    return toolChain;
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

    return m_tool->createRunControl(runConfiguration, runMode);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
