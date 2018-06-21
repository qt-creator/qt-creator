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
}

bool ConfigureStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(projectDirRelativeToBuildDir(bc) + "configure");
    pp->setArguments(additionalArguments());
    pp->resolveAll();

    return AbstractProcessStep::init(earlierSteps);
}

void ConfigureStep::run(QFutureInterface<bool>& fi)
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
        reportRunResult(fi, true);
        return;
    }

    m_runConfigure = false;
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *ConfigureStep::createConfigWidget()
{
    return new ConfigureStepConfigWidget(this);
}

bool ConfigureStep::immutable() const
{
    return false;
}

void ConfigureStep::setAdditionalArguments(const QString &list)
{
    if (list == m_additionalArguments)
        return;

    m_additionalArguments = list;
    m_runConfigure = true;

    emit additionalArgumentsChanged(list);
}

void ConfigureStep::notifyBuildDirectoryChanged()
{
    emit buildDirectoryChanged();
}

QString ConfigureStep::additionalArguments() const
{
    return m_additionalArguments;
}

QVariantMap ConfigureStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();

    map.insert(CONFIGURE_ADDITIONAL_ARGUMENTS_KEY, m_additionalArguments);
    return map;
}

bool ConfigureStep::fromMap(const QVariantMap &map)
{
    m_additionalArguments = map.value(CONFIGURE_ADDITIONAL_ARGUMENTS_KEY).toString();

    return BuildStep::fromMap(map);
}

/////////////////////////////////////
// ConfigureStepConfigWidget class
/////////////////////////////////////
ConfigureStepConfigWidget::ConfigureStepConfigWidget(ConfigureStep *configureStep) :
    m_configureStep(configureStep),
    m_additionalArguments(new QLineEdit)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    fl->addRow(tr("Arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_configureStep->additionalArguments());

    updateDetails();

    connect(m_additionalArguments, &QLineEdit::textChanged,
            configureStep, &ConfigureStep::setAdditionalArguments);
    connect(configureStep, &ConfigureStep::additionalArgumentsChanged,
            this, &ConfigureStepConfigWidget::updateDetails);
    connect(configureStep, &ConfigureStep::buildDirectoryChanged,
            this, &ConfigureStepConfigWidget::updateDetails);
}

QString ConfigureStepConfigWidget::displayName() const
{
    return tr("Configure", "AutotoolsProjectManager::ConfigureStepConfigWidget display name.");
}

QString ConfigureStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void ConfigureStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_configureStep->buildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setCommand(projectDirRelativeToBuildDir(bc) + "configure");
    param.setArguments(m_configureStep->additionalArguments());
    m_summaryText = param.summaryInWorkdir(displayName());
    emit updateSummary();
}
