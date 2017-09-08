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

#include "makestep.h"
#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"
#include "autotoolsbuildconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/qtcprocess.h>

#include <QVariantMap>
#include <QLineEdit>
#include <QFormLayout>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace AutotoolsProjectManager::Constants;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

const char MAKE_STEP_ID[] = "AutotoolsProjectManager.MakeStep";
const char CLEAN_KEY[]  = "AutotoolsProjectManager.MakeStep.Clean";
const char BUILD_TARGETS_KEY[] = "AutotoolsProjectManager.MakeStep.BuildTargets";
const char MAKE_STEP_ADDITIONAL_ARGUMENTS_KEY[] = "AutotoolsProjectManager.MakeStep.AdditionalArguments";

//////////////////////////
// MakeStepFactory class
//////////////////////////
MakeStepFactory::MakeStepFactory(QObject *parent) : IBuildStepFactory(parent)
{ setObjectName("Autotools::MakeStepFactory"); }

QList<BuildStepInfo> MakeStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != AUTOTOOLS_PROJECT_ID)
        return {};

    return {{MAKE_STEP_ID, tr("Make", "Display name for AutotoolsProjectManager::MakeStep id.")}};
}

BuildStep *MakeStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id)
    return new MakeStep(parent);
}

BuildStep *MakeStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new MakeStep(parent, static_cast<MakeStep *>(source));
}

/////////////////////
// MakeStep class
/////////////////////
MakeStep::MakeStep(BuildStepList* bsl) : AbstractProcessStep(bsl, Core::Id(MAKE_STEP_ID))
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, Core::Id id) : AbstractProcessStep(bsl, id)
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, MakeStep *bs) : AbstractProcessStep(bsl, bs),
    m_buildTargets(bs->m_buildTargets),
    m_additionalArguments(bs->additionalArguments()),
    m_clean(bs->m_clean)
{
    ctor();
}

void MakeStep::ctor()
{
    setDefaultDisplayName(tr("Make"));
}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool MakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    QList<ToolChain *> tcList = ToolChainKitInformation::toolChains(target()->kit());
    if (tcList.isEmpty())
        emit addTask(Task::compilerMissingTask());

    if (tcList.isEmpty() || !bc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    QString arguments = Utils::QtcProcess::joinArgs(m_buildTargets);
    Utils::QtcProcess::addArgs(&arguments, additionalArguments());

    setIgnoreReturnValue(m_clean);

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(tcList.at(0)->makeCommand(bc->environment()));
    pp->setArguments(arguments);
    pp->resolveAll();

    setOutputParser(new GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

void MakeStep::run(QFutureInterface<bool> &interface)
{
    AbstractProcessStep::run(interface);
}

BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

bool MakeStep::immutable() const
{
    return false;
}

void MakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if (!on && old.contains(target))
        old.removeOne(target);

    m_buildTargets = old;
}

void MakeStep::setAdditionalArguments(const QString &list)
{
    if (list == m_additionalArguments)
        return;

    m_additionalArguments = list;

    emit additionalArgumentsChanged(list);
}

QString MakeStep::additionalArguments() const
{
    return m_additionalArguments;
}

QVariantMap MakeStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();

    map.insert(BUILD_TARGETS_KEY, m_buildTargets);
    map.insert(MAKE_STEP_ADDITIONAL_ARGUMENTS_KEY, m_additionalArguments);
    map.insert(CLEAN_KEY, m_clean);
    return map;
}

bool MakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(BUILD_TARGETS_KEY).toStringList();
    m_additionalArguments = map.value(MAKE_STEP_ADDITIONAL_ARGUMENTS_KEY).toString();
    m_clean = map.value(CLEAN_KEY).toBool();

    return BuildStep::fromMap(map);
}

///////////////////////////////
// MakeStepConfigWidget class
///////////////////////////////
MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep) :
    m_makeStep(makeStep),
    m_summaryText(),
    m_additionalArguments(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_makeStep->additionalArguments());

    updateDetails();

    connect(m_additionalArguments, &QLineEdit::textChanged,
            makeStep, &MakeStep::setAdditionalArguments);
    connect(makeStep, &MakeStep::additionalArgumentsChanged,
            this, &MakeStepConfigWidget::updateDetails);
    m_makeStep->project()->subscribeSignal(&BuildConfiguration::environmentChanged, this, [this]() {
        if (static_cast<BuildConfiguration *>(sender())->isActive())
            updateDetails();
    });
    connect(makeStep->project(), &Project::activeProjectConfigurationChanged,
            this, [this](ProjectConfiguration *pc) {
        if (pc && pc->isActive())
            updateDetails();
    });
}

QString MakeStepConfigWidget::displayName() const
{
    return tr("Make", "AutotoolsProjectManager::MakeStepConfigWidget display name.");
}

QString MakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void MakeStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();
    QList<ToolChain *> tcList = ToolChainKitInformation::toolChains(m_makeStep->target()->kit());

    if (!tcList.isEmpty()) {
        QString arguments = Utils::QtcProcess::joinArgs(m_makeStep->m_buildTargets);
        Utils::QtcProcess::addArgs(&arguments, m_makeStep->additionalArguments());

        ProcessParameters param;
        param.setMacroExpander(m_makeStep->macroExpander());
        param.setEnvironment(bc->environment());
        param.setWorkingDirectory(bc->buildDirectory().toString());
        param.setCommand(tcList.at(0)->makeCommand(bc->environment()));
        param.setArguments(arguments);
        m_summaryText = param.summary(displayName());
    } else {
        m_summaryText = "<b>" + ToolChainKitInformation::msgNoToolChainInTarget()  + "</b>";
    }

    emit updateSummary();
}
