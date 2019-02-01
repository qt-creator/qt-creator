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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <QDateTime>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

const char AUTOGEN_ADDITIONAL_ARGUMENTS_KEY[] = "AutotoolsProjectManager.AutogenStep.AdditionalArguments";
const char AUTOGEN_STEP_ID[] = "AutotoolsProjectManager.AutogenStep";


// AutogenStepFactory

AutogenStepFactory::AutogenStepFactory()
{
    registerStep<AutogenStep>(AUTOGEN_STEP_ID);
    setDisplayName(AutogenStep::tr("Autogen", "Display name for AutotoolsProjectManager::AutogenStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}


// AutogenStep

AutogenStep::AutogenStep(BuildStepList *bsl) : AbstractProcessStep(bsl, AUTOGEN_STEP_ID)
{
    setDefaultDisplayName(tr("Autogen"));

    m_additionalArgumentsAspect = addAspect<BaseStringAspect>();
    m_additionalArgumentsAspect->setSettingsKey(AUTOGEN_ADDITIONAL_ARGUMENTS_KEY);
    m_additionalArgumentsAspect->setLabelText(tr("Arguments:"));
    m_additionalArgumentsAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_additionalArgumentsAspect->setHistoryCompleter("AutotoolsPM.History.AutogenStepArgs");
}

bool AutogenStep::init()
{
    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
    pp->setWorkingDirectory(projectDir);
    pp->setCommand("./autogen.sh");
    pp->setArguments(m_additionalArgumentsAspect->value());
    pp->resolveAll();

    return AbstractProcessStep::init();
}

void AutogenStep::doRun()
{
    BuildConfiguration *bc = buildConfiguration();

    // Check whether we need to run autogen.sh
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
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

BuildStepConfigWidget *AutogenStep::createConfigWidget()
{
    auto widget = AbstractProcessStep::createConfigWidget();

    auto updateDetails = [this, widget] {
        BuildConfiguration *bc = buildConfiguration();

        ProcessParameters param;
        param.setMacroExpander(bc->macroExpander());
        param.setEnvironment(bc->environment());
        const QString projectDir(bc->target()->project()->projectDirectory().toString());
        param.setWorkingDirectory(projectDir);
        param.setCommand("./autogen.sh");
        param.setArguments(m_additionalArgumentsAspect->value());

        widget->setSummaryText(param.summary(displayName()));
    };

    updateDetails();

    connect(m_additionalArgumentsAspect, &ProjectConfigurationAspect::changed, this, [=] {
        updateDetails();
        m_runAutogen = true;
    });

    return widget;
}
