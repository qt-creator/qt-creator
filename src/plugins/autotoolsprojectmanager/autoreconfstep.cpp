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

#include "autoreconfstep.h"

#include "autotoolsprojectconstants.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace AutotoolsProjectManager {
namespace Internal {

// AutoreconfStep class

/**
 * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
 *
 * A autoreconf step can be configured by selecting the "Projects" button
 * of Qt Creator (in the left hand side menu) and under "Build Settings".
 *
 * It is possible for the user to specify custom arguments.
 */

class AutoreconfStep : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(AutotoolsProjectManager::Internal::AutoreconfStep)

public:
    AutoreconfStep(BuildStepList *bsl, Utils::Id id);

    bool init() override;
    void doRun() override;

private:
    BaseStringAspect *m_additionalArgumentsAspect = nullptr;
    bool m_runAutoreconf = false;
};

AutoreconfStep::AutoreconfStep(BuildStepList *bsl, Utils::Id id)
    : AbstractProcessStep(bsl, id)
{
    setDefaultDisplayName(tr("Autoreconf"));

    m_additionalArgumentsAspect = addAspect<BaseStringAspect>();
    m_additionalArgumentsAspect->setSettingsKey("AutotoolsProjectManager.AutoreconfStep.AdditionalArguments");
    m_additionalArgumentsAspect->setLabelText(tr("Arguments:"));
    m_additionalArgumentsAspect->setValue("--force --install");
    m_additionalArgumentsAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_additionalArgumentsAspect->setHistoryCompleter("AutotoolsPM.History.AutoreconfStepArgs");

    connect(m_additionalArgumentsAspect, &ProjectConfigurationAspect::changed, this, [this] {
        m_runAutoreconf = true;
    });

    setSummaryUpdater([this] {
        ProcessParameters param;
        param.setMacroExpander(macroExpander());
        param.setEnvironment(buildEnvironment());
        param.setWorkingDirectory(project()->projectDirectory());
        param.setCommandLine({Utils::FilePath::fromString("autoreconf"),
                              m_additionalArgumentsAspect->value(),
                              Utils::CommandLine::Raw});

        return param.summary(displayName());
    });
}

bool AutoreconfStep::init()
{
    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(macroExpander());
    pp->setEnvironment(buildEnvironment());
    pp->setWorkingDirectory(project()->projectDirectory());
    pp->setCommandLine({Utils::FilePath::fromString("autoreconf"),
                        m_additionalArgumentsAspect->value(), Utils::CommandLine::Raw});

    return AbstractProcessStep::init();
}

void AutoreconfStep::doRun()
{
    // Check whether we need to run autoreconf
    const QString projectDir(project()->projectDirectory().toString());

    if (!QFileInfo::exists(projectDir + "/configure"))
        m_runAutoreconf = true;

    if (!m_runAutoreconf) {
        emit addOutput(tr("Configuration unchanged, skipping autoreconf step."), BuildStep::OutputFormat::NormalMessage);
        emit finished(true);
        return;
    }

    m_runAutoreconf = false;
    AbstractProcessStep::doRun();
}

// AutoreconfStepFactory class

/**
 * @brief Implementation of the ProjectExplorer::IBuildStepFactory interface.
 *
 * The factory is used to create instances of AutoreconfStep.
 */

AutoreconfStepFactory::AutoreconfStepFactory()
{
    registerStep<AutoreconfStep>(Constants::AUTORECONF_STEP_ID);
    setDisplayName(AutoreconfStep::tr("Autoreconf", "Display name for AutotoolsProjectManager::AutoreconfStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // Internal
} // AutotoolsProjectManager
