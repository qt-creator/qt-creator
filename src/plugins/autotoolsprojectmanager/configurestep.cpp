/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
ConfigureStepFactory::ConfigureStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

QList<Core::Id> ConfigureStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(CONFIGURE_STEP_ID);
}

QString ConfigureStepFactory::displayNameForId(Core::Id id) const
{
    if (id == CONFIGURE_STEP_ID)
        return tr("Configure", "Display name for AutotoolsProjectManager::ConfigureStep id.");
    return QString();
}

bool ConfigureStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    return canHandle(parent) && id == CONFIGURE_STEP_ID;
}

BuildStep *ConfigureStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new ConfigureStep(parent);
}

bool ConfigureStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *ConfigureStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new ConfigureStep(parent, static_cast<ConfigureStep *>(source));
}

bool ConfigureStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *ConfigureStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    ConfigureStep *bs = new ConfigureStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

bool ConfigureStepFactory::canHandle(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::AUTOTOOLS_PROJECT_ID)
        return parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD;
    return false;
}

////////////////////////
// ConfigureStep class
////////////////////////
ConfigureStep::ConfigureStep(BuildStepList* bsl) :
    AbstractProcessStep(bsl, Core::Id(CONFIGURE_STEP_ID)),
    m_runConfigure(false)
{
    ctor();
}

ConfigureStep::ConfigureStep(BuildStepList *bsl, Core::Id id) :
    AbstractProcessStep(bsl, id)
{
    ctor();
}

ConfigureStep::ConfigureStep(BuildStepList *bsl, ConfigureStep *bs) :
    AbstractProcessStep(bsl, bs),
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

void ConfigureStep::run(QFutureInterface<bool>& interface)
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
        emit addOutput(tr("Configuration unchanged, skipping configure step."), BuildStep::MessageOutput);
        interface.reportResult(true);
        emit finished();
        return;
    }

    m_runConfigure = false;
    AbstractProcessStep::run(interface);
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
    m_summaryText(),
    m_additionalArguments(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_configureStep->additionalArguments());

    updateDetails();

    connect(m_additionalArguments, SIGNAL(textChanged(QString)),
            configureStep, SLOT(setAdditionalArguments(QString)));
    connect(configureStep, SIGNAL(additionalArgumentsChanged(QString)),
            this, SLOT(updateDetails()));
    connect(configureStep, SIGNAL(buildDirectoryChanged()),
            this, SLOT(updateDetails()));
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
