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
        return QLatin1String("./");
    if (!projDirToBuildDir.endsWith(QLatin1Char('/')))
        projDirToBuildDir.append(QLatin1Char('/'));
    return projDirToBuildDir;
}

////////////////////////////////
// ConfigureStepFactory Class
////////////////////////////////
ConfigureStepFactory::ConfigureStepFactory(QObject *parent) : IBuildStepFactory(parent)
{ }

QList<BuildStepInfo> ConfigureStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != Constants::AUTOTOOLS_PROJECT_ID
            || parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return {};

    QString display = tr("Configure", "Display name for AutotoolsProjectManager::ConfigureStep id.");
    return {{CONFIGURE_STEP_ID, display}};
}

BuildStep *ConfigureStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id)
    return new ConfigureStep(parent);
}

BuildStep *ConfigureStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new ConfigureStep(parent, static_cast<ConfigureStep *>(source));
}


////////////////////////
// ConfigureStep class
////////////////////////
ConfigureStep::ConfigureStep(BuildStepList* bsl) :
    AbstractProcessStep(bsl, Core::Id(CONFIGURE_STEP_ID))
{
    ctor();
}

ConfigureStep::ConfigureStep(BuildStepList *bsl, Core::Id id) : AbstractProcessStep(bsl, id)
{
    ctor();
}

ConfigureStep::ConfigureStep(BuildStepList *bsl, ConfigureStep *bs) : AbstractProcessStep(bsl, bs),
    m_additionalArguments(bs->additionalArguments())
{
    ctor();
}

void ConfigureStep::ctor()
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
    pp->setCommand(projectDirRelativeToBuildDir(bc) + QLatin1String("configure"));
    pp->setArguments(additionalArguments());
    pp->resolveAll();

    return AbstractProcessStep::init(earlierSteps);
}

void ConfigureStep::run(QFutureInterface<bool>& fi)
{
    BuildConfiguration *bc = buildConfiguration();

    //Check whether we need to run configure
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
    const QFileInfo configureInfo(projectDir + QLatin1String("/configure"));
    const QFileInfo configStatusInfo(bc->buildDirectory().toString() + QLatin1String("/config.status"));

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

    map.insert(QLatin1String(CONFIGURE_ADDITIONAL_ARGUMENTS_KEY), m_additionalArguments);
    return map;
}

bool ConfigureStep::fromMap(const QVariantMap &map)
{
    m_additionalArguments = map.value(QLatin1String(CONFIGURE_ADDITIONAL_ARGUMENTS_KEY)).toString();

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
    param.setCommand(projectDirRelativeToBuildDir(bc) + QLatin1String("configure"));
    param.setArguments(m_configureStep->additionalArguments());
    m_summaryText = param.summaryInWorkdir(displayName());
    emit updateSummary();
}
