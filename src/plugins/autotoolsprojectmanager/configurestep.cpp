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

#include "configurestep.h"
#include "autotoolsproject.h"
#include "autotoolsbuildconfiguration.h"
#include "autotoolsprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcprocess.h>

#include <QVariantMap>
#include <QDateTime>
#include <QLineEdit>
#include <QFormLayout>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

const char CONFIGURE_ADDITIONAL_ARGUMENTS_KEY[] = "AutotoolsProjectManager.ConfigureStep.AdditionalArguments";
const char CONFIGURE_STEP_ID[] = "AutotoolsProjectManager.ConfigureStep";

/////////////////////
// Helper Function
/////////////////////
static QString projectDirRelativeToBuildDir(BuildConfiguration *bc) {
    const QDir buildDir(bc->buildDirectory().toString());
    QString projDirToBuildDir = buildDir.relativeFilePath(
                bc->target()->project()->projectDirectory().toString());
    if (projDirToBuildDir.isEmpty())
        return QString("./");
    if (!projDirToBuildDir.endsWith('/'))
        projDirToBuildDir.append('/');
    return projDirToBuildDir;
}


// ConfigureStepFactory

ConfigureStepFactory::ConfigureStepFactory()
{
    registerStep<ConfigureStep>(CONFIGURE_STEP_ID);
    setDisplayName(ConfigureStep::tr("Configure", "Display name for AutotoolsProjectManager::ConfigureStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}


// ConfigureStep

ConfigureStep::ConfigureStep(BuildStepList *bsl) : AbstractProcessStep(bsl, CONFIGURE_STEP_ID)
{
    setDefaultDisplayName(tr("Configure"));

    m_additionalArgumentsAspect = addAspect<BaseStringAspect>();
    m_additionalArgumentsAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_additionalArgumentsAspect->setSettingsKey(CONFIGURE_ADDITIONAL_ARGUMENTS_KEY);
    m_additionalArgumentsAspect->setLabelText(tr("Arguments:"));
    m_additionalArgumentsAspect->setHistoryCompleter("AutotoolsPM.History.ConfigureArgs");
}

bool ConfigureStep::init()
{
    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(projectDirRelativeToBuildDir(bc) + "configure");
    pp->setArguments(m_additionalArgumentsAspect->value());
    pp->resolveAll();

    return AbstractProcessStep::init();
}

void ConfigureStep::doRun()
{
    BuildConfiguration *bc = buildConfiguration();

    //Check whether we need to run configure
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
    const QFileInfo configureInfo(projectDir + "/configure");
    const QFileInfo configStatusInfo(bc->buildDirectory().toString() + "/config.status");

    if (!configStatusInfo.exists()
        || configStatusInfo.lastModified() < configureInfo.lastModified()) {
        m_runConfigure = true;
    }

    if (!m_runConfigure) {
        emit addOutput(tr("Configuration unchanged, skipping configure step."), BuildStep::OutputFormat::NormalMessage);
        emit finished(true);
        return;
    }

    m_runConfigure = false;
    AbstractProcessStep::doRun();
}

BuildStepConfigWidget *ConfigureStep::createConfigWidget()
{
    m_widget = AbstractProcessStep::createConfigWidget();

    updateDetails();

    connect(m_additionalArgumentsAspect, &ProjectConfigurationAspect::changed, this, [this] {
        m_runConfigure = true;
        updateDetails();
    });

    return m_widget.data();
}

void ConfigureStep::notifyBuildDirectoryChanged()
{
    updateDetails();
}

void ConfigureStep::updateDetails()
{
    if (!m_widget)
        return;

    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setCommand(projectDirRelativeToBuildDir(bc) + "configure");
    param.setArguments(m_additionalArgumentsAspect->value());

    m_widget->setSummaryText(param.summaryInWorkdir(displayName()));
}
