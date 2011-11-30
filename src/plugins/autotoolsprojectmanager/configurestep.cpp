/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "configurestep.h"
#include "autotoolsproject.h"
#include "autotoolstarget.h"
#include "autotoolsbuildconfiguration.h"
#include "autotoolsprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcprocess.h>

#include <QtCore/QVariantMap>
#include <QtCore/QDateTime>
#include <QtGui/QLineEdit>
#include <QtGui/QFormLayout>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

const char CONFIGURE_ADDITIONAL_ARGUMENTS_KEY[] = "AutotoolsProjectManager.ConfigureStep.AdditionalArguments";
const char CONFIGURE_STEP_ID[] = "AutotoolsProjectManager.ConfigureStep";

////////////////////////////////
// ConfigureStepFactory Class
////////////////////////////////
ConfigureStepFactory::ConfigureStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

QStringList ConfigureStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == QLatin1String(Constants::AUTOTOOLS_PROJECT_ID))
        return QStringList() << QLatin1String(CONFIGURE_STEP_ID);
    return QStringList();
}

QString ConfigureStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(CONFIGURE_STEP_ID))
        return tr("Configure", "Display name for AutotoolsProjectManager::ConfigureStep id.");
    return QString();
}

bool ConfigureStepFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    if (parent->target()->project()->id() != QLatin1String(Constants::AUTOTOOLS_PROJECT_ID))
        return false;

    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;

    return QLatin1String(CONFIGURE_STEP_ID) == id;
}

BuildStep *ConfigureStepFactory::create(BuildStepList *parent, const QString &id)
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

bool ConfigureStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
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

////////////////////////
// ConfigureStep class
////////////////////////
ConfigureStep::ConfigureStep(ProjectExplorer::BuildStepList* bsl) :
    AbstractProcessStep(bsl, QLatin1String(CONFIGURE_STEP_ID)),
    m_runConfigure(false)
{
    ctor();
}

ConfigureStep::ConfigureStep(ProjectExplorer::BuildStepList *bsl, const QString &id) :
    AbstractProcessStep(bsl, id)
{
    ctor();
}

ConfigureStep::ConfigureStep(ProjectExplorer::BuildStepList *bsl, ConfigureStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_additionalArguments(bs->additionalArguments())
{
    ctor();
}

void ConfigureStep::ctor()
{
    setDefaultDisplayName(tr("Configure"));
}

AutotoolsBuildConfiguration *ConfigureStep::autotoolsBuildConfiguration() const
{
    return static_cast<AutotoolsBuildConfiguration *>(buildConfiguration());
}

bool ConfigureStep::init()
{
    AutotoolsBuildConfiguration *bc = autotoolsBuildConfiguration();

    setEnabled(true);

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommand("configure");
    pp->setArguments(additionalArguments());

    return AbstractProcessStep::init();
}

void ConfigureStep::run(QFutureInterface<bool>& interface)
{
    AutotoolsBuildConfiguration *bc = autotoolsBuildConfiguration();

    //Check whether we need to run configure
    const QFileInfo configureInfo(bc->buildDirectory() + QLatin1String("/configure"));
    const QFileInfo configStatusInfo(bc->buildDirectory() + QLatin1String("/config.status"));

    if (!configStatusInfo.exists()
        || configStatusInfo.lastModified() < configureInfo.lastModified()) {
        m_runConfigure = true;
    }

    if (!m_runConfigure) {
        emit addOutput(tr("Configuration unchanged, skipping configure step."), BuildStep::MessageOutput);
        interface.reportResult(true);
        return;
    }

    m_runConfigure = false;
    AbstractProcessStep::run(interface);
}

ProjectExplorer::BuildStepConfigWidget* ConfigureStep::createConfigWidget()
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
    AutotoolsBuildConfiguration *bc = m_configureStep->autotoolsBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    param.setWorkingDirectory(bc->buildDirectory());
    param.setCommand("configure");
    param.setArguments(m_configureStep->additionalArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}
