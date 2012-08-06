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

#include "autoreconfstep.h"
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
#include <QLineEdit>
#include <QFormLayout>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

const char AUTORECONF_STEP_ID[] = "AutotoolsProjectManager.AutoreconfStep";
const char AUTORECONF_ADDITIONAL_ARGUMENTS_KEY[] = "AutotoolsProjectManager.AutoreconfStep.AdditionalArguments";

////////////////////////////////
// AutoreconfStepFactory class
////////////////////////////////
AutoreconfStepFactory::AutoreconfStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

QList<Core::Id> AutoreconfStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(AUTORECONF_STEP_ID);
}

QString AutoreconfStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == AUTORECONF_STEP_ID)
        return tr("Autoreconf", "Display name for AutotoolsProjectManager::AutoreconfStep id.");
    return QString();
}

bool AutoreconfStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    return canHandle(parent) && Core::Id(AUTORECONF_STEP_ID) == id;
}

BuildStep *AutoreconfStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new AutoreconfStep(parent);
}

bool AutoreconfStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *AutoreconfStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new AutoreconfStep(parent, static_cast<AutoreconfStep *>(source));
}

bool AutoreconfStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *AutoreconfStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AutoreconfStep *bs = new AutoreconfStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

bool AutoreconfStepFactory::canHandle(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::AUTOTOOLS_PROJECT_ID)
        return parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD;
    return false;
}

/////////////////////////
// AutoreconfStep class
/////////////////////////
AutoreconfStep::AutoreconfStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(AUTORECONF_STEP_ID)),
    m_runAutoreconf(false)
{
    ctor();
}

AutoreconfStep::AutoreconfStep(BuildStepList *bsl, const Core::Id id) :
    AbstractProcessStep(bsl, id)
{
    ctor();
}

AutoreconfStep::AutoreconfStep(BuildStepList *bsl, AutoreconfStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_additionalArguments(bs->additionalArguments())
{
    ctor();
}

void AutoreconfStep::ctor()
{
    setDefaultDisplayName(tr("Autoreconf"));
}

AutotoolsBuildConfiguration *AutoreconfStep::autotoolsBuildConfiguration() const
{
    return static_cast<AutotoolsBuildConfiguration *>(buildConfiguration());
}

bool AutoreconfStep::init()
{
    AutotoolsBuildConfiguration *bc = autotoolsBuildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommand(QLatin1String("autoreconf"));
    pp->setArguments(additionalArguments());

    return AbstractProcessStep::init();
}

void AutoreconfStep::run(QFutureInterface<bool> &interface)
{
    AutotoolsBuildConfiguration *bc = autotoolsBuildConfiguration();

    // Check whether we need to run autoreconf
    const QFileInfo configureInfo(bc->buildDirectory() + QLatin1String("/configure"));

    if (!configureInfo.exists())
        m_runAutoreconf = true;

    if (!m_runAutoreconf) {
        emit addOutput(tr("Configuration unchanged, skipping autoreconf step."), BuildStep::MessageOutput);
        interface.reportResult(true);
        return;
    }

    m_runAutoreconf = false;
    AbstractProcessStep::run(interface);
}

BuildStepConfigWidget *AutoreconfStep::createConfigWidget()
{
    return new AutoreconfStepConfigWidget(this);
}

bool AutoreconfStep::immutable() const
{
    return false;
}

void AutoreconfStep::setAdditionalArguments(const QString &list)
{
    if (list == m_additionalArguments)
        return;

    m_additionalArguments = list;
    m_runAutoreconf = true;

    emit additionalArgumentsChanged(list);
}

QString AutoreconfStep::additionalArguments() const
{
    return m_additionalArguments;
}

QVariantMap AutoreconfStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();

    map.insert(QLatin1String(AUTORECONF_ADDITIONAL_ARGUMENTS_KEY), m_additionalArguments);
    return map;
}

bool AutoreconfStep::fromMap(const QVariantMap &map)
{
    m_additionalArguments = map.value(QLatin1String(AUTORECONF_ADDITIONAL_ARGUMENTS_KEY)).toString();

    return BuildStep::fromMap(map);
}

//////////////////////////////////////
// AutoreconfStepConfigWidget class
//////////////////////////////////////
AutoreconfStepConfigWidget::AutoreconfStepConfigWidget(AutoreconfStep *autoreconfStep) :
    m_autoreconfStep(autoreconfStep),
    m_summaryText(),
    m_additionalArguments(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_autoreconfStep->additionalArguments());

    updateDetails();

    connect(m_additionalArguments, SIGNAL(textChanged(QString)),
            autoreconfStep, SLOT(setAdditionalArguments(QString)));
    connect(autoreconfStep, SIGNAL(additionalArgumentsChanged(QString)),
            this, SLOT(updateDetails()));
}

QString AutoreconfStepConfigWidget::displayName() const
{
    return tr("Autoreconf", "AutotoolsProjectManager::AutoreconfStepConfigWidget display name.");
}

QString AutoreconfStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void AutoreconfStepConfigWidget::updateDetails()
{
    AutotoolsBuildConfiguration *bc = m_autoreconfStep->autotoolsBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    param.setWorkingDirectory(bc->buildDirectory());
    param.setCommand(QLatin1String("autoreconf"));
    param.setArguments(m_autoreconfStep->additionalArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}
