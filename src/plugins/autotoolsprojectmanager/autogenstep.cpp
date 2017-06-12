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

const char AUTOGEN_ADDITIONAL_ARGUMENTS_KEY[] = "AutotoolsProjectManager.AutogenStep.AdditionalArguments";
const char AUTOGEN_STEP_ID[] = "AutotoolsProjectManager.AutogenStep";

/////////////////////////////
// AutogenStepFactory class
/////////////////////////////
AutogenStepFactory::AutogenStepFactory(QObject *parent) : IBuildStepFactory(parent)
{ }

QList<BuildStepInfo> AutogenStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != Constants::AUTOTOOLS_PROJECT_ID
            || parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return {};

    QString display = tr("Autogen", "Display name for AutotoolsProjectManager::AutogenStep id.");
    return {{AUTOGEN_STEP_ID, display}};
}

BuildStep *AutogenStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id)
    return new AutogenStep(parent);
}

BuildStep *AutogenStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new AutogenStep(parent, static_cast<AutogenStep *>(source));
}

////////////////////////
// AutogenStep class
////////////////////////
AutogenStep::AutogenStep(BuildStepList *bsl) : AbstractProcessStep(bsl, Core::Id(AUTOGEN_STEP_ID))
{
    ctor();
}

AutogenStep::AutogenStep(BuildStepList *bsl, Core::Id id) : AbstractProcessStep(bsl, id)
{
    ctor();
}

AutogenStep::AutogenStep(BuildStepList *bsl, AutogenStep *bs) : AbstractProcessStep(bsl, bs),
    m_additionalArguments(bs->additionalArguments())
{
    ctor();
}

void AutogenStep::ctor()
{
    setDefaultDisplayName(tr("Autogen"));
}

bool AutogenStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
    pp->setWorkingDirectory(projectDir);
    pp->setCommand("./autogen.sh");
    pp->setArguments(additionalArguments());
    pp->resolveAll();

    return AbstractProcessStep::init(earlierSteps);
}

void AutogenStep::run(QFutureInterface<bool> &fi)
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
        reportRunResult(fi, true);
        return;
    }

    m_runAutogen = false;
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *AutogenStep::createConfigWidget()
{
    return new AutogenStepConfigWidget(this);
}

bool AutogenStep::immutable() const
{
    return false;
}

void AutogenStep::setAdditionalArguments(const QString &list)
{
    if (list == m_additionalArguments)
        return;

    m_additionalArguments = list;
    m_runAutogen = true;

    emit additionalArgumentsChanged(list);
}

QString AutogenStep::additionalArguments() const
{
    return m_additionalArguments;
}

QVariantMap AutogenStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(AUTOGEN_ADDITIONAL_ARGUMENTS_KEY, m_additionalArguments);
    return map;
}

bool AutogenStep::fromMap(const QVariantMap &map)
{
    m_additionalArguments = map.value(AUTOGEN_ADDITIONAL_ARGUMENTS_KEY).toString();

    return BuildStep::fromMap(map);
}

//////////////////////////////////
// AutogenStepConfigWidget class
//////////////////////////////////
AutogenStepConfigWidget::AutogenStepConfigWidget(AutogenStep *autogenStep) :
    m_autogenStep(autogenStep),
    m_additionalArguments(new QLineEdit)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    fl->addRow(tr("Arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_autogenStep->additionalArguments());

    updateDetails();

    connect(m_additionalArguments, &QLineEdit::textChanged,
            autogenStep, &AutogenStep::setAdditionalArguments);
    connect(autogenStep, &AutogenStep::additionalArgumentsChanged,
            this, &AutogenStepConfigWidget::updateDetails);
}

QString AutogenStepConfigWidget::displayName() const
{
    return tr("Autogen", "AutotoolsProjectManager::AutogenStepConfigWidget display name.");
}

QString AutogenStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void AutogenStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_autogenStep->buildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    const QString projectDir(bc->target()->project()->projectDirectory().toString());
    param.setWorkingDirectory(projectDir);
    param.setCommand("./autogen.sh");
    param.setArguments(m_autogenStep->additionalArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}
