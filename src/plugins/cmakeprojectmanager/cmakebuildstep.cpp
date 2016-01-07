/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "cmakebuildstep.h"

#include "cmakebuildconfiguration.h"
#include "cmakeparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakerunconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>

#include <coreplugin/find/itemviewfind.h>

#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>

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
const char CLEAN_KEY[] = "CMakeProjectManager.MakeStep.Clean"; // Obsolete since QtC 3.7
const char BUILD_TARGETS_KEY[] = "CMakeProjectManager.MakeStep.BuildTargets";
const char ADDITIONAL_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.AdditionalArguments";
const char ADD_RUNCONFIGURATION_ARGUMENT_KEY[] = "CMakeProjectManager.MakeStep.AddRunConfigurationArgument";
const char MAKE_COMMAND_KEY[] = "CMakeProjectManager.MakeStep.MakeCommand";
const char ADD_RUNCONFIGURATION_TEXT[] = "Current executable";
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(MS_ID)), m_addRunConfigurationArgument(false)
{
    ctor();
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl, Core::Id id) :
    AbstractProcessStep(bsl, id), m_addRunConfigurationArgument(false)
{
    ctor();
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl, CMakeBuildStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_buildTargets(bs->m_buildTargets),
    m_additionalArguments(bs->m_additionalArguments),
    m_addRunConfigurationArgument(bs->m_addRunConfigurationArgument),
    m_makeCmd(bs->m_makeCmd)
{
    ctor();
}

void CMakeBuildStep::ctor()
{
    m_percentProgress = QRegExp(QLatin1String("^\\[\\s*(\\d*)%\\]"));
    m_ninjaProgress = QRegExp(QLatin1String("^\\[\\s*(\\d*)/\\s*(\\d*)"));
    m_ninjaProgressString = QLatin1String("[%f/%t "); // ninja: [33/100
    //: Default display name for the cmake make step.
    setDefaultDisplayName(tr("Make"));

    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    if (bc) {
        m_activeConfiguration = 0;
        connect(bc, &CMakeBuildConfiguration::useNinjaChanged, this, &CMakeBuildStep::makeCommandChanged);
    } else {
        // That means the step is in the deploylist, so we listen to the active build config
        // changed signal and react to the activeBuildConfigurationChanged() signal of the buildconfiguration
        m_activeConfiguration = targetsActiveBuildConfiguration();
        connect(target(), &Target::activeBuildConfigurationChanged, this, &CMakeBuildStep::activeBuildConfigurationChanged);
        activeBuildConfigurationChanged();
    }

    connect(static_cast<CMakeProject *>(project()), &CMakeProject::buildTargetsChanged,
            this, &CMakeBuildStep::buildTargetsChanged);
}

CMakeBuildConfiguration *CMakeBuildStep::cmakeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(buildConfiguration());
}

CMakeBuildConfiguration *CMakeBuildStep::targetsActiveBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(target()->activeBuildConfiguration());
}

CMakeRunConfiguration *CMakeBuildStep::targetsActiveRunConfiguration() const
{
    return qobject_cast<CMakeRunConfiguration *>(target()->activeRunConfiguration());
}

void CMakeBuildStep::activeBuildConfigurationChanged()
{
    if (m_activeConfiguration)
        disconnect(m_activeConfiguration, &CMakeBuildConfiguration::useNinjaChanged, this, &CMakeBuildStep::makeCommandChanged);

    m_activeConfiguration = targetsActiveBuildConfiguration();

    if (m_activeConfiguration)
        connect(m_activeConfiguration, &CMakeBuildConfiguration::useNinjaChanged, this, &CMakeBuildStep::makeCommandChanged);

    emit makeCommandChanged();
}

void CMakeBuildStep::buildTargetsChanged()
{
    QStringList filteredTargets;
    foreach (const QString &t, static_cast<CMakeProject *>(project())->buildTargetTitles()) {
        if (m_buildTargets.contains(t))
            filteredTargets.append(t);
    }
    setBuildTargets(filteredTargets);
}

QVariantMap CMakeBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(ADDITIONAL_ARGUMENTS_KEY), m_additionalArguments);
    map.insert(QLatin1String(ADD_RUNCONFIGURATION_ARGUMENT_KEY), m_addRunConfigurationArgument);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCmd);
    return map;
}

bool CMakeBuildStep::fromMap(const QVariantMap &map)
{
    if (map.value(QLatin1String(CLEAN_KEY), false).toBool()) {
        m_buildTargets = QStringList({ CMakeBuildStep::cleanTarget() });
    } else {
        m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
        m_additionalArguments = map.value(QLatin1String(ADDITIONAL_ARGUMENTS_KEY)).toString();
    }
    m_addRunConfigurationArgument = map.value(QLatin1String(ADD_RUNCONFIGURATION_ARGUMENT_KEY), false).toBool();
    m_makeCmd = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

    return BuildStep::fromMap(map);
}


bool CMakeBuildStep::init(QList<const BuildStep *> &earlierSteps)
{
    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();

    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    m_useNinja = bc->useNinja();

    QString arguments;
    if (m_addRunConfigurationArgument) {
        CMakeRunConfiguration* rc = targetsActiveRunConfiguration();
        if (!rc) {
            emit addTask(Task(Task::Error,
                              QCoreApplication::translate("ProjectExplorer::Task",
                                        "You asked to build the current Run Configurations build target only, "
                                        "but the current Run Configuration is not associated with a build target. "
                                        "Please update the Make Step in your build settings."),
                            Utils::FileName(), -1,
                            ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
            emitFaultyConfigurationMessage();
            return false;
        }
        if (!rc->title().isEmpty())
            Utils::QtcProcess::addArg(&arguments, rc->title());
    }
    Utils::QtcProcess::addArgs(&arguments, m_buildTargets);
    Utils::QtcProcess::addArgs(&arguments, additionalArguments());

    setIgnoreReturnValue(m_buildTargets.contains(CMakeBuildStep::cleanTarget()));

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    if (m_useNinja && !env.value(QLatin1String("NINJA_STATUS")).startsWith(m_ninjaProgressString))
        env.set(QLatin1String("NINJA_STATUS"), m_ninjaProgressString + QLatin1String("%o/sec] "));
    pp->setEnvironment(env);
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(makeCommand(tc, bc->environment()));
    pp->setArguments(arguments);
    pp->resolveAll();

    setOutputParser(new CMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

void CMakeBuildStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *CMakeBuildStep::createConfigWidget()
{
    return new CMakeBuildStepConfigWidget(this);
}

bool CMakeBuildStep::immutable() const
{
    return false;
}

void CMakeBuildStep::stdOutput(const QString &line)
{
    if (m_percentProgress.indexIn(line) != -1) {
        bool ok = false;
        int percent = m_percentProgress.cap(1).toInt(&ok);
        if (ok)
            futureInterface()->setProgressValue(percent);
    } else if (m_ninjaProgress.indexIn(line) != -1) {
        bool ok = false;
        int done = m_ninjaProgress.cap(1).toInt(&ok);
        if (ok) {
            int all = m_ninjaProgress.cap(2).toInt(&ok);
            if (ok && all != 0) {
                int percent = 100.0 * done/all;
                futureInterface()->setProgressValue(percent);
            }
        }
    }
    if (m_useNinja)
        AbstractProcessStep::stdError(line);
    else
        AbstractProcessStep::stdOutput(line);
}

QStringList CMakeBuildStep::buildTargets() const
{
    return m_buildTargets;
}

bool CMakeBuildStep::buildsBuildTarget(const QString &target) const
{
    if (target == tr(ADD_RUNCONFIGURATION_TEXT))
        return addRunConfigurationArgument();
    else
        return m_buildTargets.contains(target);
}

void CMakeBuildStep::setBuildTarget(const QString &buildTarget, bool on)
{
    if (buildTarget == tr(ADD_RUNCONFIGURATION_TEXT)) {
        setAddRunConfigurationArgument(on);
    } else {
        QStringList old = m_buildTargets;
        if (on && !old.contains(buildTarget))
            old << buildTarget;
        else if (!on && old.contains(buildTarget))
            old.removeOne(buildTarget);
        setBuildTargets(old);
    }
}

void CMakeBuildStep::setBuildTargets(const QStringList &targets)
{
    if (targets != m_buildTargets) {
        m_buildTargets = targets;
        emit targetsToBuildChanged();
    }
}

void CMakeBuildStep::clearBuildTargets()
{
    m_buildTargets.clear();
}

QString CMakeBuildStep::additionalArguments() const
{
    return m_additionalArguments;
}

void CMakeBuildStep::setAdditionalArguments(const QString &list)
{
    m_additionalArguments = list;
}

bool CMakeBuildStep::addRunConfigurationArgument() const
{
    return m_addRunConfigurationArgument;
}

void CMakeBuildStep::setAddRunConfigurationArgument(bool add)
{
    m_addRunConfigurationArgument = add;
}

QString CMakeBuildStep::makeCommand(ToolChain *tc, const Utils::Environment &env) const
{
    if (!m_makeCmd.isEmpty())
        return m_makeCmd;
    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();
    if (bc && bc->useNinja())
        return QLatin1String("ninja");

    if (tc)
        return tc->makeCommand(env);

    return QLatin1String("make");
}

void CMakeBuildStep::setUserMakeCommand(const QString &make)
{
    m_makeCmd = make;
}

QString CMakeBuildStep::userMakeCommand() const
{
    return m_makeCmd;
}

QString CMakeBuildStep::cleanTarget()
{
    return QLatin1String("clean");
}

//
// CMakeBuildStepConfigWidget
//

CMakeBuildStepConfigWidget::CMakeBuildStepConfigWidget(CMakeBuildStep *buildStep)
    : m_buildStep(buildStep)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_makePathChooser = new Utils::PathChooser(this);
    m_makePathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_makePathChooser->setBaseDirectory(Utils::PathChooser::homePath());
    m_makePathChooser->setHistoryCompleter(QLatin1String("PE.MakeCommand.History"));
    m_makePathChooser->setPath(m_buildStep->userMakeCommand());

    fl->addRow(tr("Override command:"), m_makePathChooser);

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Additional arguments:"), m_additionalArguments);
    m_additionalArguments->setText(m_buildStep->additionalArguments());

    m_buildTargetsList = new QListWidget;
    m_buildTargetsList->setFrameStyle(QFrame::NoFrame);
    m_buildTargetsList->setMinimumHeight(200);

    QFrame *frame = new QFrame(this);
    frame->setFrameStyle(QFrame::StyledPanel);
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setMargin(0);
    frameLayout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_buildTargetsList,
                                                                       Core::ItemViewFind::LightColored));

    fl->addRow(tr("Targets:"), frame);

    auto itemAddRunConfigurationArgument = new QListWidgetItem(tr(ADD_RUNCONFIGURATION_TEXT), m_buildTargetsList);
    itemAddRunConfigurationArgument->setFlags(itemAddRunConfigurationArgument->flags() | Qt::ItemIsUserCheckable);
    itemAddRunConfigurationArgument->setCheckState(m_buildStep->addRunConfigurationArgument() ? Qt::Checked : Qt::Unchecked);
    QFont f;
    f.setItalic(true);
    itemAddRunConfigurationArgument->setFont(f);

    CMakeProject *pro = static_cast<CMakeProject *>(m_buildStep->project());
    QStringList targetList = pro->buildTargetTitles();
    targetList.sort();
    foreach (const QString &buildTarget, targetList) {
        QListWidgetItem *item = new QListWidgetItem(buildTarget, m_buildTargetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_buildStep->buildsBuildTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    updateDetails();

    connect(m_makePathChooser, &Utils::PathChooser::rawPathChanged, this, &CMakeBuildStepConfigWidget::makeEdited);
    connect(m_additionalArguments, &QLineEdit::textEdited, this, &CMakeBuildStepConfigWidget::additionalArgumentsEdited);
    connect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &CMakeBuildStepConfigWidget::updateDetails);

    connect(pro, &CMakeProject::buildTargetsChanged, this, &CMakeBuildStepConfigWidget::buildTargetsChanged);
    connect(m_buildStep, &CMakeBuildStep::targetsToBuildChanged, this, &CMakeBuildStepConfigWidget::selectedBuildTargetsChanged);
    connect(pro, &CMakeProject::environmentChanged, this, &CMakeBuildStepConfigWidget::updateDetails);
    connect(m_buildStep, &CMakeBuildStep::makeCommandChanged, this, &CMakeBuildStepConfigWidget::updateDetails);
}

void CMakeBuildStepConfigWidget::makeEdited()
{
    m_buildStep->setUserMakeCommand(m_makePathChooser->rawPath());
    updateDetails();
}

void CMakeBuildStepConfigWidget::additionalArgumentsEdited()
{
    m_buildStep->setAdditionalArguments(m_additionalArguments->text());
    updateDetails();
}

void CMakeBuildStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_buildStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

QString CMakeBuildStepConfigWidget::displayName() const
{
    return tr("Make", "CMakeProjectManager::CMakeBuildStepConfigWidget display name.");
}

void CMakeBuildStepConfigWidget::buildTargetsChanged()
{
    disconnect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);

    auto *addRunConfigurationArgumentItem = m_buildTargetsList->takeItem(0);
    m_buildTargetsList->clear();
    m_buildTargetsList->insertItem(0, addRunConfigurationArgumentItem);

    CMakeProject *pro = static_cast<CMakeProject *>(m_buildStep->target()->project());
    foreach (const QString& buildTarget, pro->buildTargetTitles()) {
        QListWidgetItem *item = new QListWidgetItem(buildTarget, m_buildTargetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_buildStep->buildsBuildTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    connect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);
    updateSummary();
}

void CMakeBuildStepConfigWidget::selectedBuildTargetsChanged()
{
    disconnect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);
    for (int y = 0; y < m_buildTargetsList->count(); ++y) {
        QListWidgetItem *item = m_buildTargetsList->item(y);
        item->setCheckState(m_buildStep->buildsBuildTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    connect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);
    updateSummary();
}

void CMakeBuildStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_buildStep->buildConfiguration();
    if (!bc)
        bc = m_buildStep->target()->activeBuildConfiguration();
    if (!bc) {
        m_summaryText = tr("<b>No build configuration found on this kit.</b>");
        updateSummary();
        return;
    }

    ToolChain *tc = ToolChainKitInformation::toolChain(m_buildStep->target()->kit());
    if (tc) {
        QString arguments;
        if (m_buildStep->addRunConfigurationArgument())
            arguments = QLatin1String("<i>&lt;") + tr(ADD_RUNCONFIGURATION_TEXT) + QLatin1String("&gt;</i>");

        Utils::QtcProcess::addArgs(&arguments, Utils::QtcProcess::joinArgs(m_buildStep->buildTargets()));
        Utils::QtcProcess::addArgs(&arguments, m_buildStep->additionalArguments());

        ProcessParameters param;
        param.setMacroExpander(bc->macroExpander());
        param.setEnvironment(bc->environment());
        param.setWorkingDirectory(bc->buildDirectory().toString());
        param.setCommand(m_buildStep->makeCommand(tc, bc->environment()));
        param.setArguments(arguments);
        m_summaryText = param.summary(displayName());
    } else {
        m_summaryText = QLatin1String("<b>") + ToolChainKitInformation::msgNoToolChainInTarget() + QLatin1String("</b>");
    }
    emit updateSummary();
}

QString CMakeBuildStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

//
// CMakeBuildStepFactory
//

CMakeBuildStepFactory::CMakeBuildStepFactory(QObject *parent) : IBuildStepFactory(parent)
{
}

CMakeBuildStepFactory::~CMakeBuildStepFactory()
{
}

bool CMakeBuildStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::CMAKEPROJECT_ID)
        return id == MS_ID;
    return false;
}

BuildStep *CMakeBuildStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    CMakeBuildStep *step = new CMakeBuildStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        step->setBuildTarget(CMakeBuildStep::cleanTarget(), true);
    return step;
}

bool CMakeBuildStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *CMakeBuildStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new CMakeBuildStep(parent, static_cast<CMakeBuildStep *>(source));
}

bool CMakeBuildStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *CMakeBuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CMakeBuildStep *bs(new CMakeBuildStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> CMakeBuildStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::CMAKEPROJECT_ID)
        return QList<Core::Id>() << Core::Id(MS_ID);
    return QList<Core::Id>();
}

QString CMakeBuildStepFactory::displayNameForId(Core::Id id) const
{
    if (id == MS_ID)
        return tr("Make", "Display name for CMakeProjectManager::CMakeBuildStep id.");
    return QString();
}

void CMakeBuildStep::processStarted()
{
    futureInterface()->setProgressRange(0, 100);
    AbstractProcessStep::processStarted();
}

void CMakeBuildStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    futureInterface()->setProgressValue(100);
}
