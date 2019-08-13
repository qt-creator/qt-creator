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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

// AutoreconfStepFactory class

AutoreconfStepFactory::AutoreconfStepFactory()
{
    registerStep<AutoreconfStep>(Constants::AUTORECONF_STEP_ID);
    setDisplayName(AutoreconfStep::tr("Autoreconf", "Display name for AutotoolsProjectManager::AutoreconfStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}


// AutoreconfStep class

AutoreconfStep::AutoreconfStep(BuildStepList *bsl)
    : AbstractProcessStep(bsl, Constants::AUTORECONF_STEP_ID)
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
        BuildConfiguration *bc = buildConfiguration();

        ProcessParameters param;
        param.setMacroExpander(bc->macroExpander());
        param.setEnvironment(bc->environment());
        param.setWorkingDirectory(bc->target()->project()->projectDirectory());
        param.setCommandLine({Utils::FilePath::fromString("autoreconf"),
                              m_additionalArgumentsAspect->value(),
                              Utils::CommandLine::Raw});

        return param.summary(displayName());
    });
}

bool AutoreconfStep::init()
{
    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->target()->project()->projectDirectory());
    pp->setCommandLine({Utils::FilePath::fromString("autoreconf"),
                        m_additionalArgumentsAspect->value(), Utils::CommandLine::Raw});

    return AbstractProcessStep::init();
}

void AutoreconfStep::doRun()
{
    BuildConfiguration *bc = buildConfiguration();

    // Check whether we need to run autoreconf
    const QString projectDir(bc->target()->project()->projectDirectory().toString());

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
