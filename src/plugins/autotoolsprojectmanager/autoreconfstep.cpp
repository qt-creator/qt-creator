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
AutoreconfStepFactory::AutoreconfStepFactory(QObject *parent) : IBuildStepFactory(parent)
{ }

QList<BuildStepInfo> AutoreconfStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != Constants::AUTOTOOLS_PROJECT_ID
            || parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return {};

    QString display = tr("Autoreconf", "Display name for AutotoolsProjectManager::AutoreconfStep id.");
    return {{AUTORECONF_STEP_ID, display}};
}

BuildStep *AutoreconfStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id);
    return new AutoreconfStep(parent);
}

BuildStep *AutoreconfStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new AutoreconfStep(parent, static_cast<AutoreconfStep *>(source));
}

/////////////////////////
// AutoreconfStep class
/////////////////////////
AutoreconfStep::AutoreconfStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(AUTORECONF_STEP_ID))
{
    ctor();
}

AutoreconfStep::AutoreconfStep(BuildStepList *bsl, Core::Id id) : AbstractProcessStep(bsl, id)
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

bool AutoreconfStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
    pp->setWorkingDirectory(projectDir);
    pp->setCommand(QLatin1String("autoreconf"));
    pp->setArguments(additionalArguments());
    pp->resolveAll();

    return AbstractProcessStep::init(earlierSteps);
}

void AutoreconfStep::run(QFutureInterface<bool> &fi)
{
    BuildConfiguration *bc = buildConfiguration();

    // Check whether we need to run autoreconf
    const QString projectDir(bc->target()->project()->projectDirectory().toString());

    if (!QFileInfo::exists(projectDir + QLatin1String("/configure")))
        m_runAutoreconf = true;

    if (!m_runAutoreconf) {
        emit addOutput(tr("Configuration unchanged, skipping autoreconf step."), BuildStep::OutputFormat::NormalMessage);
        reportRunResult(fi, true);
        return;
    }

    m_runAutoreconf = false;
    AbstractProcessStep::run(fi);
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
    m_additionalArguments(new QLineEdit(this))
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    fl->addRow(tr("Arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_autoreconfStep->additionalArguments());

    updateDetails();

    connect(m_additionalArguments, &QLineEdit::textChanged,
            autoreconfStep, &AutoreconfStep::setAdditionalArguments);
    connect(autoreconfStep, &AutoreconfStep::additionalArgumentsChanged,
            this, &AutoreconfStepConfigWidget::updateDetails);
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
    BuildConfiguration *bc = m_autoreconfStep->buildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
    param.setWorkingDirectory(projectDir);
    param.setCommand(QLatin1String("autoreconf"));
    param.setArguments(m_autoreconfStep->additionalArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}
