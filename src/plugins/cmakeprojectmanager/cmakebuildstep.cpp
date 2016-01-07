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
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakerunconfiguration.h"
#include "cmaketool.h"

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

#include <utils/algorithm.h>
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
const char TOOL_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.AdditionalArguments";
const char ADD_RUNCONFIGURATION_ARGUMENT_KEY[] = "CMakeProjectManager.MakeStep.AddRunConfigurationArgument";
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
    m_toolArguments(bs->m_toolArguments),
    m_addRunConfigurationArgument(bs->m_addRunConfigurationArgument)
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

    connect(target(), &Target::kitChanged, this, &CMakeBuildStep::cmakeCommandChanged);
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

void CMakeBuildStep::buildTargetsChanged()
{
    const QStringList filteredTargets
            = Utils::filtered(static_cast<CMakeProject *>(project())->buildTargetTitles(),
                              [this](const QString &s) { return m_buildTargets.contains(s); });
    setBuildTargets(filteredTargets);
}

QVariantMap CMakeBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(TOOL_ARGUMENTS_KEY), m_toolArguments);
    map.insert(QLatin1String(ADD_RUNCONFIGURATION_ARGUMENT_KEY), m_addRunConfigurationArgument);
    return map;
}

bool CMakeBuildStep::fromMap(const QVariantMap &map)
{
    if (map.value(QLatin1String(CLEAN_KEY), false).toBool()) {
        m_buildTargets = {CMakeBuildStep::cleanTarget()};
    } else {
        m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
        m_toolArguments = map.value(QLatin1String(TOOL_ARGUMENTS_KEY)).toString();
    }
    m_addRunConfigurationArgument = map.value(QLatin1String(ADD_RUNCONFIGURATION_ARGUMENT_KEY), false).toBool();

    return BuildStep::fromMap(map);
}


bool CMakeBuildStep::init(QList<const BuildStep *> &earlierSteps)
{
    bool canInit = true;
    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();
    if (!bc) {
        emit addTask(Task::buildConfigurationMissingTask());
        canInit = false;
    }

    CMakeTool *tool = CMakeKitInformation::cmakeTool(target()->kit());
    if (!tool || !tool->isValid()) {
        emit addTask(Task(Task::Error,
                          QCoreApplication::translate("CMakeProjectManager::CMakeBuildStep",
                                                      "Qt Creator needs a cmake tool set up to build. "
                                                      "Configure a cmake tool in the kit options."),
                          Utils::FileName(), -1,
                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        canInit = false;
    }

    CMakeRunConfiguration *rc = targetsActiveRunConfiguration();
    if (m_addRunConfigurationArgument && (!rc || rc->title().isEmpty())) {
        emit addTask(Task(Task::Error,
                          QCoreApplication::translate("ProjectExplorer::Task",
                                    "You asked to build the current Run Configurations build target only, "
                                    "but the current Run Configuration is not associated with a build target. "
                                    "Please update the Make Step in your build settings."),
                        Utils::FileName(), -1,
                        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        canInit = false;
    }

    if (!canInit) {
        emitFaultyConfigurationMessage();
        return false;
    }

    QString arguments = allArguments(rc);

    setIgnoreReturnValue(m_buildTargets.contains(CMakeBuildStep::cleanTarget()));

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    if (!env.value(QLatin1String("NINJA_STATUS")).startsWith(m_ninjaProgressString))
        env.set(QLatin1String("NINJA_STATUS"), m_ninjaProgressString + QLatin1String("%o/sec] "));
    pp->setEnvironment(env);
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(cmakeCommand());
    pp->setArguments(arguments);
    pp->resolveAll();

    setOutputParser(new CMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
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
        m_useNinja = true;
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

QString CMakeBuildStep::toolArguments() const
{
    return m_toolArguments;
}

void CMakeBuildStep::setToolArguments(const QString &list)
{
    m_toolArguments = list;
}

QString CMakeBuildStep::allArguments(const CMakeRunConfiguration *rc) const
{
    QString arguments;

    Utils::QtcProcess::addArg(&arguments, QLatin1String("--build"));
    Utils::QtcProcess::addArg(&arguments, QLatin1String("."));

    if (m_addRunConfigurationArgument) {
        Utils::QtcProcess::addArg(&arguments, QLatin1String("--target"));
        if (rc)
            Utils::QtcProcess::addArg(&arguments, rc->title());
        else
            Utils::QtcProcess::addArg(&arguments, QLatin1String("<i>&lt;") + tr(ADD_RUNCONFIGURATION_TEXT) + QLatin1String("&gt;</i>"));
    }
    foreach (const QString &t, m_buildTargets) {
        Utils::QtcProcess::addArg(&arguments, QLatin1String("--target"));
        Utils::QtcProcess::addArg(&arguments, t);
    }

    if (!m_toolArguments.isEmpty()) {
        Utils::QtcProcess::addArg(&arguments, QLatin1String("--"));
        Utils::QtcProcess::addArg(&arguments, m_toolArguments);
    }

    return arguments;
}

bool CMakeBuildStep::addRunConfigurationArgument() const
{
    return m_addRunConfigurationArgument;
}

void CMakeBuildStep::setAddRunConfigurationArgument(bool add)
{
    m_addRunConfigurationArgument = add;
}

QString CMakeBuildStep::cmakeCommand() const
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(target()->kit());
    return tool ? tool->cmakeExecutable().toString() : QString();
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

    m_toolArguments = new QLineEdit(this);
    fl->addRow(tr("Tool arguments:"), m_toolArguments);
    m_toolArguments->setText(m_buildStep->toolArguments());

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

    connect(m_toolArguments, &QLineEdit::textEdited, this, &CMakeBuildStepConfigWidget::toolArgumentsEdited);
    connect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &CMakeBuildStepConfigWidget::updateDetails);

    connect(pro, &CMakeProject::buildTargetsChanged, this, &CMakeBuildStepConfigWidget::buildTargetsChanged);
    connect(m_buildStep, &CMakeBuildStep::targetsToBuildChanged, this, &CMakeBuildStepConfigWidget::selectedBuildTargetsChanged);
    connect(pro, &CMakeProject::environmentChanged, this, &CMakeBuildStepConfigWidget::updateDetails);
}

void CMakeBuildStepConfigWidget::toolArgumentsEdited()
{
    m_buildStep->setToolArguments(m_toolArguments->text());
    updateDetails();
}

void CMakeBuildStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_buildStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

QString CMakeBuildStepConfigWidget::displayName() const
{
    return tr("Build", "CMakeProjectManager::CMakeBuildStepConfigWidget display name.");
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
        bc = m_buildStep->targetsActiveBuildConfiguration();
    if (!bc) {
        m_summaryText = tr("<b>No build configuration found on this kit.</b>");
        updateSummary();
        return;
    }

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setEnvironment(bc->environment());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setCommand(m_buildStep->cmakeCommand());
    param.setArguments(m_buildStep->allArguments(0));
    m_summaryText = param.summary(displayName());

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
{ }

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
        return tr("Build", "Display name for CMakeProjectManager::CMakeBuildStep id.");
    return QString();
}

void CMakeBuildStep::processStarted()
{
    m_useNinja = false;
    futureInterface()->setProgressRange(0, 100);
    AbstractProcessStep::processStarted();
}

void CMakeBuildStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    futureInterface()->setProgressValue(100);
}
