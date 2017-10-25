/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <projectexplorer/gnumakeparser.h>
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
const char CLEAN_KEY[] = "CMakeProjectManager.MakeStep.Clean"; // Obsolete since QtC 3.7
const char BUILD_TARGETS_KEY[] = "CMakeProjectManager.MakeStep.BuildTargets";
const char TOOL_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.AdditionalArguments";
const char ADD_RUNCONFIGURATION_ARGUMENT_KEY[] = "CMakeProjectManager.MakeStep.AddRunConfigurationArgument";
const char ADD_RUNCONFIGURATION_TEXT[] = "Current executable";
}

static bool isCurrentExecutableTarget(const QString &target)
{
    return target == ADD_RUNCONFIGURATION_TEXT;
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(Constants::CMAKE_BUILD_STEP_ID))
{
    ctor(bsl);
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl, Core::Id id) : AbstractProcessStep(bsl, id)
{
    ctor(bsl);
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl, CMakeBuildStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_buildTarget(bs->m_buildTarget),
    m_toolArguments(bs->m_toolArguments)
{
    ctor(bsl);
}

void CMakeBuildStep::ctor(BuildStepList *bsl)
{
    m_percentProgress = QRegExp("^\\[\\s*(\\d*)%\\]");
    m_ninjaProgress = QRegExp("^\\[\\s*(\\d*)/\\s*(\\d*)");
    m_ninjaProgressString = "[%f/%t "; // ninja: [33/100
    //: Default display name for the cmake make step.
    setDefaultDisplayName(tr("CMake Build"));

    auto bc = qobject_cast<CMakeBuildConfiguration *>(bsl->parent());
    if (!bc) {
        auto t = qobject_cast<Target *>(bsl->parent()->parent());
        QTC_ASSERT(t, return);
        bc = qobject_cast<CMakeBuildConfiguration *>(t->activeBuildConfiguration());
    }

    // Set a good default build target:
    if (m_buildTarget.isEmpty()) {
        if (bsl->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
            setBuildTarget(cleanTarget());
        else if (bsl->id() == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            setBuildTarget(installTarget());
        else
            setBuildTarget(allTarget());
    }

    connect(target(), &Target::kitChanged, this, &CMakeBuildStep::cmakeCommandChanged);
    connect(project(), &Project::parsingFinished,
            this, &CMakeBuildStep::handleBuildTargetChanges);
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

void CMakeBuildStep::handleBuildTargetChanges()
{
    if (isCurrentExecutableTarget(m_buildTarget))
        return; // Do not change just because a different set of build targets is there...
    if (!static_cast<CMakeProject *>(project())->buildTargetTitles().contains(m_buildTarget))
        setBuildTarget(allTarget());
    emit buildTargetsChanged();
}

QVariantMap CMakeBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    // Use QStringList for compatibility with old files
    map.insert(BUILD_TARGETS_KEY, QStringList(m_buildTarget));
    map.insert(TOOL_ARGUMENTS_KEY, m_toolArguments);
    return map;
}

bool CMakeBuildStep::fromMap(const QVariantMap &map)
{
    if (map.value(CLEAN_KEY, false).toBool()) {
        m_buildTarget = CMakeBuildStep::cleanTarget();
    } else {
        const QStringList targetList = map.value(BUILD_TARGETS_KEY).toStringList();
        if (!targetList.isEmpty())
            m_buildTarget = targetList.last();
        m_toolArguments = map.value(TOOL_ARGUMENTS_KEY).toString();
    }
    if (map.value(ADD_RUNCONFIGURATION_ARGUMENT_KEY, false).toBool())
        m_buildTarget = ADD_RUNCONFIGURATION_TEXT;

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
    if (bc && !bc->isEnabled()) {
        emit addTask(Task(Task::Error,
                          QCoreApplication::translate("CMakeProjectManager::CMakeBuildStep",
                                                      "The build configuration is currently disabled."),
                          Utils::FileName(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        canInit = false;
    }

    CMakeTool *tool = CMakeKitInformation::cmakeTool(target()->kit());
    if (!tool || !tool->isValid()) {
        emit addTask(Task(Task::Error,
                          tr("A CMake tool must be set up for building. "
                             "Configure a CMake tool in the kit options."),
                          Utils::FileName(), -1,
                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        canInit = false;
    }

    CMakeRunConfiguration *rc = targetsActiveRunConfiguration();
    if (isCurrentExecutableTarget(m_buildTarget) && (!rc || rc->buildSystemTarget().isEmpty())) {
        emit addTask(Task(Task::Error,
                          QCoreApplication::translate("ProjectExplorer::Task",
                                    "You asked to build the current Run Configuration's build target only, "
                                    "but it is not associated with a build target. "
                                    "Update the Make Step in your build settings."),
                        Utils::FileName(), -1,
                        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        canInit = false;
    }

    if (!canInit) {
        emitFaultyConfigurationMessage();
        return false;
    }

    // Warn if doing out-of-source builds with a CMakeCache.txt is the source directory
    const Utils::FileName projectDirectory = bc->target()->project()->projectDirectory();
    if (bc->buildDirectory() != projectDirectory) {
        Utils::FileName cmc = projectDirectory;
        cmc.appendPath("CMakeCache.txt");
        if (cmc.exists()) {
            emit addTask(Task(Task::Warning,
                              tr("There is a CMakeCache.txt file in \"%1\", which suggest an "
                                 "in-source build was done before. You are now building in \"%2\", "
                                 "and the CMakeCache.txt file might confuse CMake.")
                              .arg(projectDirectory.toUserOutput(), bc->buildDirectory().toUserOutput()),
                              Utils::FileName(), -1,
                              ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }

    QString arguments = allArguments(rc);

    setIgnoreReturnValue(m_buildTarget == CMakeBuildStep::cleanTarget());

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    if (!env.value("NINJA_STATUS").startsWith(m_ninjaProgressString))
        env.set("NINJA_STATUS", m_ninjaProgressString + "%o/sec] ");
    pp->setEnvironment(env);
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(cmakeCommand());
    pp->setArguments(arguments);
    pp->resolveAll();

    setOutputParser(new CMakeParser);
    appendOutputParser(new GnuMakeParser);
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

void CMakeBuildStep::run(QFutureInterface<bool> &fi)
{
    // Make sure CMake state was written to disk before trying to build:
    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();
    QTC_ASSERT(bc, return);

    bool mustDelay = false;
    auto p = static_cast<CMakeProject *>(bc->project());
    if (p->persistCMakeState()) {
        emit addOutput(tr("Persisting CMake state..."), BuildStep::OutputFormat::NormalMessage);
        mustDelay = true;
    } else if (p->mustUpdateCMakeStateBeforeBuild()) {
        emit addOutput(tr("Running CMake in preparation to build..."), BuildStep::OutputFormat::NormalMessage);
        mustDelay = true;
    } else {
        mustDelay = false;
    }

    if (mustDelay) {
        m_runTrigger = connect(project(), &Project::parsingFinished,
                               this, [this, &fi](bool success) { handleProjectWasParsed(fi, success); });
    } else {
        runImpl(fi);
    }
}

void CMakeBuildStep::runImpl(QFutureInterface<bool> &fi)
{
    // Do the actual build:
    AbstractProcessStep::run(fi);
}

void CMakeBuildStep::handleProjectWasParsed(QFutureInterface<bool> &fi, bool success)
{
    disconnect(m_runTrigger);
    if (success) {
        runImpl(fi);
    } else {
        AbstractProcessStep::stdError(tr("Project did not parse successfully, cannot build."));
        reportRunResult(fi, false);
    }
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
        AbstractProcessStep::stdOutput(line);
        bool ok = false;
        int percent = m_percentProgress.cap(1).toInt(&ok);
        if (ok)
            futureInterface()->setProgressValue(percent);
        return;
    } else if (m_ninjaProgress.indexIn(line) != -1) {
        AbstractProcessStep::stdOutput(line);
        m_useNinja = true;
        bool ok = false;
        int done = m_ninjaProgress.cap(1).toInt(&ok);
        if (ok) {
            int all = m_ninjaProgress.cap(2).toInt(&ok);
            if (ok && all != 0) {
                const int percent = static_cast<int>(100.0 * done/all);
                futureInterface()->setProgressValue(percent);
            }
        }
        return;
    }
    if (m_useNinja)
        AbstractProcessStep::stdError(line);
    else
        AbstractProcessStep::stdOutput(line);
}

QString CMakeBuildStep::buildTarget() const
{
    return m_buildTarget;
}

bool CMakeBuildStep::buildsBuildTarget(const QString &target) const
{
    return target == m_buildTarget;
}

void CMakeBuildStep::setBuildTarget(const QString &buildTarget)
{
    if (m_buildTarget == buildTarget)
        return;
    m_buildTarget = buildTarget;
    emit targetToBuildChanged();
}

void CMakeBuildStep::clearBuildTargets()
{
    m_buildTarget.clear();
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

    Utils::QtcProcess::addArg(&arguments, "--build");
    Utils::QtcProcess::addArg(&arguments, ".");

    QString target;

    if (isCurrentExecutableTarget(m_buildTarget)) {
        if (rc)
            target = rc->buildSystemTarget();
        else
            target = "<i>&lt;" + tr(ADD_RUNCONFIGURATION_TEXT) + "&gt;</i>";
    } else {
        target = m_buildTarget;
    }

    Utils::QtcProcess::addArg(&arguments, "--target");
    Utils::QtcProcess::addArg(&arguments, target);

    if (!m_toolArguments.isEmpty()) {
        Utils::QtcProcess::addArg(&arguments, "--");
        arguments += ' ' + m_toolArguments;
    }

    return arguments;
}

QString CMakeBuildStep::cmakeCommand() const
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(target()->kit());
    return tool ? tool->cmakeExecutable().toString() : QString();
}

QString CMakeBuildStep::cleanTarget()
{
    return QString("clean");
}

QString CMakeBuildStep::allTarget()
{
    return QString("all");
}

QString CMakeBuildStep::installTarget()
{
    return QString("install");
}

QString CMakeBuildStep::testTarget()
{
    return QString("test");
}

QStringList CMakeBuildStep::specialTargets()
{
    return { allTarget(), cleanTarget(), installTarget(), testTarget() };
}

//
// CMakeBuildStepConfigWidget
//

CMakeBuildStepConfigWidget::CMakeBuildStepConfigWidget(CMakeBuildStep *buildStep) :
    m_buildStep(buildStep),
    m_toolArguments(new QLineEdit),
    m_buildTargetsList(new QListWidget)
{
    auto fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    fl->addRow(tr("Tool arguments:"), m_toolArguments);
    m_toolArguments->setText(m_buildStep->toolArguments());

    m_buildTargetsList->setFrameStyle(QFrame::NoFrame);
    m_buildTargetsList->setMinimumHeight(200);

    auto frame = new QFrame(this);
    frame->setFrameStyle(QFrame::StyledPanel);
    auto frameLayout = new QVBoxLayout(frame);
    frameLayout->setMargin(0);
    frameLayout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_buildTargetsList,
                                                                       Core::ItemViewFind::LightColored));

    fl->addRow(tr("Targets:"), frame);

    buildTargetsChanged();
    updateDetails();

    connect(m_toolArguments, &QLineEdit::textEdited, this, &CMakeBuildStepConfigWidget::toolArgumentsEdited);
    connect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &CMakeBuildStepConfigWidget::updateDetails);

    connect(m_buildStep, &CMakeBuildStep::buildTargetsChanged, this, &CMakeBuildStepConfigWidget::buildTargetsChanged);
    connect(m_buildStep, &CMakeBuildStep::targetToBuildChanged, this, &CMakeBuildStepConfigWidget::selectedBuildTargetsChanged);
    m_buildStep->project()->subscribeSignal(&BuildConfiguration::environmentChanged, this, [this]() {
        if (static_cast<BuildConfiguration *>(sender())->isActive())
            updateDetails();
    });
    connect(m_buildStep->project(), &Project::activeProjectConfigurationChanged,
            this, [this](ProjectConfiguration *pc) {
        if (pc && pc->isActive())
            updateDetails();
    });
}

void CMakeBuildStepConfigWidget::toolArgumentsEdited()
{
    m_buildStep->setToolArguments(m_toolArguments->text());
    updateDetails();
}

void CMakeBuildStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    const QString target =
            (item->checkState() == Qt::Checked) ? item->data(Qt::UserRole).toString() : CMakeBuildStep::allTarget();
    m_buildStep->setBuildTarget(target);
    updateDetails();
}

QString CMakeBuildStepConfigWidget::displayName() const
{
    return tr("Build", "CMakeProjectManager::CMakeBuildStepConfigWidget display name.");
}

void CMakeBuildStepConfigWidget::buildTargetsChanged()
{
    {
        QSignalBlocker blocker(m_buildTargetsList);
        m_buildTargetsList->clear();

        auto pro = static_cast<CMakeProject *>(m_buildStep->project());
        QStringList targetList = pro->buildTargetTitles();
        targetList.sort();

        QFont italics;
        italics.setItalic(true);

        auto exeItem = new QListWidgetItem(tr(ADD_RUNCONFIGURATION_TEXT), m_buildTargetsList);
        exeItem->setData(Qt::UserRole, ADD_RUNCONFIGURATION_TEXT);

        foreach (const QString &buildTarget, targetList) {
            auto item = new QListWidgetItem(buildTarget, m_buildTargetsList);
            item->setData(Qt::UserRole, buildTarget);
        }

        for (int i = 0; i < m_buildTargetsList->count(); ++i) {
            QListWidgetItem *item = m_buildTargetsList->item(i);
            const QString title = item->data(Qt::UserRole).toString();

            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(m_buildStep->buildsBuildTarget(title) ? Qt::Checked : Qt::Unchecked);

            // Print utility targets in italics:
            if (CMakeBuildStep::specialTargets().contains(title) || title == ADD_RUNCONFIGURATION_TEXT)
                item->setFont(italics);
        }
    }
    updateDetails();
}

void CMakeBuildStepConfigWidget::selectedBuildTargetsChanged()
{
    {
        QSignalBlocker blocker(m_buildTargetsList);
        for (int y = 0; y < m_buildTargetsList->count(); ++y) {
            QListWidgetItem *item = m_buildTargetsList->item(y);
            item->setCheckState(m_buildStep->buildsBuildTarget(item->data(Qt::UserRole).toString())
                                ? Qt::Checked : Qt::Unchecked);
        }
    }
    updateDetails();
}

void CMakeBuildStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_buildStep->buildConfiguration();
    if (!bc)
        bc = m_buildStep->targetsActiveBuildConfiguration();
    if (!bc) {
        m_summaryText = tr("<b>No build configuration found on this kit.</b>");
        emit updateSummary();
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

QList<BuildStepInfo> CMakeBuildStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != Constants::CMAKEPROJECT_ID)
        return {};

    return {{Constants::CMAKE_BUILD_STEP_ID,
             tr("Build", "Display name for CMakeProjectManager::CMakeBuildStep id.")}};
}

BuildStep *CMakeBuildStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id);
    return new CMakeBuildStep(parent);
}

BuildStep *CMakeBuildStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new CMakeBuildStep(parent, static_cast<CMakeBuildStep *>(source));
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
