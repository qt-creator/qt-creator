/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "makestep.h"

#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>

#include <utils/qtcprocess.h>

#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char MS_ID[] = "CMakeProjectManager.MakeStep";
const char CLEAN_KEY[] = "CMakeProjectManager.MakeStep.Clean";
const char BUILD_TARGETS_KEY[] = "CMakeProjectManager.MakeStep.BuildTargets";
const char ADDITIONAL_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.AdditionalArguments";
const char USE_NINJA_KEY[] = "CMakeProjectManager.MakeStep.UseNinja";
}

MakeStep::MakeStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(MS_ID)), m_clean(false),
    m_futureInterface(0)
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, const Core::Id id) :
    AbstractProcessStep(bsl, id), m_clean(false),
    m_futureInterface(0)
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, MakeStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_clean(bs->m_clean),
    m_futureInterface(0),
    m_buildTargets(bs->m_buildTargets),
    m_additionalArguments(Utils::QtcProcess::joinArgs(bs->m_buildTargets))
{
    ctor();
}

void MakeStep::ctor()
{
    m_percentProgress = QRegExp(QLatin1String("^\\[\\s*(\\d*)%\\]"));
    m_ninjaProgress = QRegExp(QLatin1String("^\\[\\s*(\\d*)/\\s*(\\d*)"));
    m_ninjaProgressString = QLatin1String("[%s/%t "); // ninja: [33/100
    //: Default display name for the cmake make step.
    setDefaultDisplayName(tr("Make"));

    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    if (bc) {
        m_useNinja = bc->useNinja();
        m_activeConfiguration = 0;
        connect(bc, SIGNAL(useNinjaChanged(bool)), this, SLOT(setUseNinja(bool)));
    } else {
        // That means the step is in the deploylist, so we listen to the active build config
        // changed signal and react to the activeBuildConfigurationChanged() signal of the buildconfiguration
        m_activeConfiguration = targetsActiveBuildConfiguration();
        m_useNinja = m_activeConfiguration->useNinja();
        connect (target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                 this, SLOT(activeBuildConfigurationChanged()));
        activeBuildConfigurationChanged();
    }
}

MakeStep::~MakeStep()
{
}

CMakeBuildConfiguration *MakeStep::cmakeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(buildConfiguration());
}

CMakeBuildConfiguration *MakeStep::targetsActiveBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(target()->activeBuildConfiguration());
}

void MakeStep::activeBuildConfigurationChanged()
{
    if (m_activeConfiguration)
        disconnect(m_activeConfiguration, SIGNAL(useNinjaChanged(bool)), this, SLOT(setUseNinja(bool)));

    m_activeConfiguration = targetsActiveBuildConfiguration();

    if (m_activeConfiguration) {
        connect(m_activeConfiguration, SIGNAL(useNinjaChanged(bool)), this, SLOT(setUseNinja(bool)));
        setUseNinja(m_activeConfiguration->useNinja());
    }
}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

QVariantMap MakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(ADDITIONAL_ARGUMENTS_KEY), m_additionalArguments);
    map.insert(QLatin1String(USE_NINJA_KEY), m_useNinja);
    return map;
}

bool MakeStep::fromMap(const QVariantMap &map)
{
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();
    m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
    m_additionalArguments = map.value(QLatin1String(ADDITIONAL_ARGUMENTS_KEY)).toString();
    m_useNinja = map.value(QLatin1String(USE_NINJA_KEY)).toBool();

    return BuildStep::fromMap(map);
}


bool MakeStep::init()
{
    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    if (!bc)
        bc = static_cast<CMakeBuildConfiguration *>(target()->activeBuildConfiguration());

    m_tasks.clear();
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc) {
        m_tasks.append(Task(Task::Error, tr("Qt Creator needs a compiler set up to build. Configure a compiler in the kit options."),
                            Utils::FileName(), -1,
                            Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return true; // otherwise the tasks will not get reported
    }

    QString arguments = Utils::QtcProcess::joinArgs(m_buildTargets);
    Utils::QtcProcess::addArgs(&arguments, additionalArguments());

    setIgnoreReturnValue(m_clean);

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    if (m_useNinja && !env.value(QLatin1String("NINJA_STATUS")).startsWith(m_ninjaProgressString))
        env.set(QLatin1String("NINJA_STATUS"), m_ninjaProgressString + QLatin1String("%o/sec] "));
    pp->setEnvironment(env);
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommand(makeCommand(tc, bc->environment()));
    pp->setArguments(arguments);
    pp->resolveAll();

    setOutputParser(new ProjectExplorer::GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
}

void MakeStep::run(QFutureInterface<bool> &fi)
{
    bool canContinue = true;
    foreach (const Task &t, m_tasks) {
        addTask(t);
        canContinue = false;
    }
    if (!canContinue) {
        emit addOutput(tr("Configuration is faulty. Check the Issues view for details."), BuildStep::MessageOutput);
        fi.reportResult(false);
        return;
    }

    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0, 100);
    AbstractProcessStep::run(fi);
    m_futureInterface->setProgressValue(100);
    m_futureInterface->reportFinished();
    m_futureInterface = 0;
}

BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

bool MakeStep::immutable() const
{
    return false;
}

void MakeStep::stdOutput(const QString &line)
{
    if (m_percentProgress.indexIn(line) != -1) {
        bool ok = false;
        int percent = m_percentProgress.cap(1).toInt(&ok);;
        if (ok)
            m_futureInterface->setProgressValue(percent);
    } else if (m_ninjaProgress.indexIn(line) != -1) {
        bool ok = false;
        int done = m_ninjaProgress.cap(1).toInt(&ok);
        if (ok) {
            int all = m_ninjaProgress.cap(2).toInt(&ok);
            if (ok && all != 0) {
                int percent = 100.0 * done/all;
                m_futureInterface->setProgressValue(percent);
            }
        }
    }
    if (m_useNinja)
        AbstractProcessStep::stdError(line);
    else
        AbstractProcessStep::stdOutput(line);
}

QStringList MakeStep::buildTargets() const
{
    return m_buildTargets;
}

bool MakeStep::buildsBuildTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void MakeStep::setBuildTarget(const QString &buildTarget, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(buildTarget))
        old << buildTarget;
    else if (!on && old.contains(buildTarget))
        old.removeOne(buildTarget);
    m_buildTargets = old;
}

void MakeStep::setBuildTargets(const QStringList &targets)
{
    m_buildTargets = targets;
}

void MakeStep::clearBuildTargets()
{
    m_buildTargets.clear();
}

QString MakeStep::additionalArguments() const
{
    return m_additionalArguments;
}

void MakeStep::setAdditionalArguments(const QString &list)
{
    m_additionalArguments = list;
}

QString MakeStep::makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const
{
    if (m_useNinja)
        return QLatin1String("ninja");
    if (tc)
        return tc->makeCommand(env);

    return QLatin1String("make");
}

void MakeStep::setUseNinja(bool useNinja)
{
    if (m_useNinja != useNinja) {
        m_useNinja = useNinja;
        emit makeCommandChanged();
    }
}

//
// MakeStepConfigWidget
//

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep)
    : m_makeStep(makeStep)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Additional arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_makeStep->additionalArguments());

    m_buildTargetsList = new QListWidget;
    m_buildTargetsList->setMinimumHeight(200);
    fl->addRow(tr("Kits:"), m_buildTargetsList);

    // TODO update this list also on rescans of the CMakeLists.txt
    CMakeProject *pro = static_cast<CMakeProject *>(m_makeStep->target()->project());
    foreach (const QString& buildTarget, pro->buildTargetTitles()) {
        QListWidgetItem *item = new QListWidgetItem(buildTarget, m_buildTargetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsBuildTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    updateDetails();

    connect(m_additionalArguments, SIGNAL(textEdited(QString)), this, SLOT(additionalArgumentsEdited()));
    connect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(pro, SIGNAL(buildTargetsChanged()),
            this, SLOT(buildTargetsChanged()));
    connect(pro, SIGNAL(environmentChanged()), this, SLOT(updateDetails()));
    connect(m_makeStep, SIGNAL(makeCommandChanged()), this, SLOT(updateDetails()));
}

void MakeStepConfigWidget::additionalArgumentsEdited()
{
    m_makeStep->setAdditionalArguments(m_additionalArguments->text());
    updateDetails();
}

void MakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

QString MakeStepConfigWidget::displayName() const
{
    return tr("Make", "CMakeProjectManager::MakeStepConfigWidget display name.");
}

void MakeStepConfigWidget::buildTargetsChanged()
{
    disconnect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    m_buildTargetsList->clear();
    CMakeProject *pro = static_cast<CMakeProject *>(m_makeStep->target()->project());
    foreach (const QString& buildTarget, pro->buildTargetTitles()) {
        QListWidgetItem *item = new QListWidgetItem(buildTarget, m_buildTargetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsBuildTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    connect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    updateSummary();
}

void MakeStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();
    if (!bc) {
        m_summaryText = tr("<b>No build configuration found on this kit.</b>");
        updateSummary();
        return;
    }

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(m_makeStep->target()->kit());
    if (tc) {
        QString arguments = Utils::QtcProcess::joinArgs(m_makeStep->buildTargets());
        Utils::QtcProcess::addArgs(&arguments, m_makeStep->additionalArguments());

        ProcessParameters param;
        param.setMacroExpander(bc->macroExpander());
        param.setEnvironment(bc->environment());
        param.setWorkingDirectory(bc->buildDirectory());
        param.setCommand(m_makeStep->makeCommand(tc, bc->environment()));
        param.setArguments(arguments);
        m_summaryText = param.summary(displayName());
    } else {
        m_summaryText = QLatin1String("<b>") + ProjectExplorer::ToolChainKitInformation::msgNoToolChainInTarget() + QLatin1String("</b>");
    }
    emit updateSummary();
}

QString MakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

//
// MakeStepFactory
//

MakeStepFactory::MakeStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

MakeStepFactory::~MakeStepFactory()
{
}

bool MakeStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::CMAKEPROJECT_ID)
        return id == MS_ID;
    return false;
}

BuildStep *MakeStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    MakeStep *step = new MakeStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setClean(true);
        step->setAdditionalArguments(QLatin1String("clean"));
    }
    return step;
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
    MakeStep *bs(new MakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> MakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::CMAKEPROJECT_ID)
        return QList<Core::Id>() << Core::Id(MS_ID);
    return QList<Core::Id>();
}

QString MakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == MS_ID)
        return tr("Make", "Display name for CMakeProjectManager::MakeStep id.");
    return QString();
}
