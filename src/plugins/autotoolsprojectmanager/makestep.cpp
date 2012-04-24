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

#include "makestep.h"
#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"
#include "autotoolsbuildconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
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
MakeStepFactory::MakeStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{ setObjectName(QLatin1String("Autotools::MakeStepFactory")); }

QList<Core::Id> MakeStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Core::Id(AUTOTOOLS_PROJECT_ID))
        return QList<Core::Id>() << Core::Id(MAKE_STEP_ID);
    return QList<Core::Id>();
}

QString MakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(MAKE_STEP_ID))
        return tr("Make", "Display name for AutotoolsProjectManager::MakeStep id.");
    return QString();
}

bool MakeStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() != Core::Id(AUTOTOOLS_PROJECT_ID))
        return false;

    return Core::Id(MAKE_STEP_ID) == id;
}

BuildStep *MakeStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new MakeStep(parent);
}

bool MakeStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *MakeStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new MakeStep(parent, static_cast<MakeStep *>(source));
}

bool MakeStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *MakeStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    MakeStep *bs = new MakeStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

/////////////////////
// MakeStep class
/////////////////////
MakeStep::MakeStep(BuildStepList* bsl) :
    AbstractProcessStep(bsl, Core::Id(MAKE_STEP_ID)),
    m_clean(false)
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, const Core::Id id) :
    AbstractProcessStep(bsl, id),
    m_clean(false)
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, MakeStep *bs) :
    AbstractProcessStep(bsl, bs),
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

AutotoolsBuildConfiguration *MakeStep::autotoolsBuildConfiguration() const
{
    return static_cast<AutotoolsBuildConfiguration *>(buildConfiguration());
}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool MakeStep::init()
{
    AutotoolsBuildConfiguration *bc = autotoolsBuildConfiguration();

    QString arguments = Utils::QtcProcess::joinArgs(m_buildTargets);
    Utils::QtcProcess::addArgs(&arguments, additionalArguments());

    setIgnoreReturnValue(m_clean);

    ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(bc->target()->profile());

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommand(tc ? tc->makeCommand() : QLatin1String("make"));
    pp->setArguments(arguments);

    setOutputParser(new GnuMakeParser());
    if (tc)
        appendOutputParser(tc->outputParser());
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
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

    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(MAKE_STEP_ADDITIONAL_ARGUMENTS_KEY), m_additionalArguments);
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    return map;
}

bool MakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
    m_additionalArguments = map.value(QLatin1String(MAKE_STEP_ADDITIONAL_ARGUMENTS_KEY)).toString();
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();

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

    connect(m_additionalArguments, SIGNAL(textChanged(QString)),
            makeStep, SLOT(setAdditionalArguments(QString)));
    connect(makeStep, SIGNAL(additionalArgumentsChanged(QString)),
            this, SLOT(updateDetails()));
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
    AutotoolsBuildConfiguration *bc = m_makeStep->autotoolsBuildConfiguration();
    ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(m_makeStep->target()->profile());

    if (tc) {
        QString arguments = Utils::QtcProcess::joinArgs(m_makeStep->m_buildTargets);
        Utils::QtcProcess::addArgs(&arguments, m_makeStep->additionalArguments());

        ProcessParameters param;
        param.setMacroExpander(bc->macroExpander());
        param.setEnvironment(bc->environment());
        param.setWorkingDirectory(bc->buildDirectory());
        param.setCommand(tc->makeCommand());
        param.setArguments(arguments);
        m_summaryText = param.summary(displayName());
    } else {
        m_summaryText = tr("<b>No tool chain set up for this profile</b>");
    }

    emit updateSummary();
}
