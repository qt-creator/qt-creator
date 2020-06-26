/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autogenstep.h"

#include "autotoolsprojectconstants.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager {
namespace Internal {

// AutogenStep

/**
 * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
 *
 * A autogen step can be configured by selecting the "Projects" button of Qt Creator
 * (in the left hand side menu) and under "Build Settings".
 *
 * It is possible for the user to specify custom arguments.
 */

class AutogenStep : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(AutotoolsProjectManager::Internal::AutogenStep)

public:
    AutogenStep(BuildStepList *bsl, Utils::Id id);

private:
    bool init() override;
    void doRun() override;

    BaseStringAspect *m_additionalArgumentsAspect = nullptr;
    bool m_runAutogen = false;
};

AutogenStep::AutogenStep(BuildStepList *bsl, Utils::Id id) : AbstractProcessStep(bsl, id)
{
    setDefaultDisplayName(tr("Autogen"));

    m_additionalArgumentsAspect = addAspect<BaseStringAspect>();
    m_additionalArgumentsAspect->setSettingsKey(
                "AutotoolsProjectManager.AutogenStep.AdditionalArguments");
    m_additionalArgumentsAspect->setLabelText(tr("Arguments:"));
    m_additionalArgumentsAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_additionalArgumentsAspect->setHistoryCompleter("AutotoolsPM.History.AutogenStepArgs");

    connect(m_additionalArgumentsAspect, &ProjectConfigurationAspect::changed, this, [this] {
        m_runAutogen = true;
    });

    setSummaryUpdater([this] {
        ProcessParameters param;
        param.setMacroExpander(macroExpander());
        param.setEnvironment(buildEnvironment());
        param.setWorkingDirectory(project()->projectDirectory());
        param.setCommandLine({FilePath::fromString("./autogen.sh"),
                              m_additionalArgumentsAspect->value(),
                              CommandLine::Raw});

        return param.summary(displayName());
    });
}

bool AutogenStep::init()
{
    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(macroExpander());
    pp->setEnvironment(buildEnvironment());
    pp->setWorkingDirectory(project()->projectDirectory());
    pp->setCommandLine({FilePath::fromString("./autogen.sh"),
                        m_additionalArgumentsAspect->value(),
                        CommandLine::Raw});

    return AbstractProcessStep::init();
}

void AutogenStep::doRun()
{
    // Check whether we need to run autogen.sh
    const QString projectDir = project()->projectDirectory().toString();
    const QFileInfo configureInfo(projectDir + "/configure");
    const QFileInfo configureAcInfo(projectDir + "/configure.ac");
    const QFileInfo makefileAmInfo(projectDir + "/Makefile.am");

    if (!configureInfo.exists()
        || configureInfo.lastModified() < configureAcInfo.lastModified()
        || configureInfo.lastModified() < makefileAmInfo.lastModified()) {
        m_runAutogen = true;
    }

    if (!m_runAutogen) {
        emit addOutput(tr("Configuration unchanged, skipping autogen step."), BuildStep::OutputFormat::NormalMessage);
        emit finished(true);
        return;
    }

    m_runAutogen = false;
    AbstractProcessStep::doRun();
}

// AutogenStepFactory

/**
 * @brief Implementation of the ProjectExplorer::BuildStepFactory interface.
 *
 * This factory is used to create instances of AutogenStep.
 */

AutogenStepFactory::AutogenStepFactory()
{
    registerStep<AutogenStep>(Constants::AUTOGEN_STEP_ID);
    setDisplayName(AutogenStep::tr("Autogen", "Display name for AutotoolsProjectManager::AutogenStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // Internal
} // AutotoolsProjectManager
