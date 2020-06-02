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
#include "cmakebuildsystem.h"
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectconstants.h"
#include "cmaketool.h"

#include <coreplugin/find/itemviewfind.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <QBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QRadioButton>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

const char BUILD_TARGETS_KEY[] = "CMakeProjectManager.MakeStep.BuildTargets";
const char CMAKE_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.CMakeArguments";
const char TOOL_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.AdditionalArguments";
const char ADD_RUNCONFIGURATION_ARGUMENT_KEY[] = "CMakeProjectManager.MakeStep.AddRunConfigurationArgument";
const char ADD_RUNCONFIGURATION_TEXT[] = "Current executable";

class CMakeBuildStepConfigWidget : public BuildStepConfigWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeBuildStepConfigWidget)

public:
    explicit CMakeBuildStepConfigWidget(CMakeBuildStep *buildStep);

private:
    void itemChanged(QListWidgetItem *);
    void cmakeArgumentsEdited();
    void toolArgumentsEdited();
    void updateDetails();
    void buildTargetsChanged();
    void updateBuildTarget();

    QRadioButton *itemWidget(QListWidgetItem *item);

    CMakeBuildStep *m_buildStep;
    QLineEdit *m_cmakeArguments;
    QLineEdit *m_toolArguments;
    QListWidget *m_buildTargetsList;
};

static bool isCurrentExecutableTarget(const QString &target)
{
    return target == ADD_RUNCONFIGURATION_TEXT;
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl, Core::Id id) :
    AbstractProcessStep(bsl, id)
{
    m_percentProgress = QRegExp("^\\[\\s*(\\d*)%\\]");
    m_ninjaProgress = QRegExp("^\\[\\s*(\\d*)/\\s*(\\d*)");
    m_ninjaProgressString = "[%f/%t "; // ninja: [33/100
    //: Default display name for the cmake make step.
    setDefaultDisplayName(tr("CMake Build"));

    // Set a good default build target:
    if (m_buildTarget.isEmpty())
        setBuildTarget(defaultBuildTarget());

    setLowPriority();

    connect(target(), &Target::parsingFinished,
            this, &CMakeBuildStep::handleBuildTargetChanges);
}

CMakeBuildConfiguration *CMakeBuildStep::cmakeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(buildConfiguration());
}

void CMakeBuildStep::handleBuildTargetChanges(bool success)
{
    if (!success)
        return; // Do not change when parsing failed.
    if (!isCurrentExecutableTarget(m_buildTarget) && !knownBuildTargets().contains(m_buildTarget)) {
        setBuildTarget(defaultBuildTarget());
    }
    emit buildTargetsChanged();
}

QVariantMap CMakeBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    // Use QStringList for compatibility with old files
    map.insert(BUILD_TARGETS_KEY, QStringList(m_buildTarget));
    map.insert(CMAKE_ARGUMENTS_KEY, m_cmakeArguments);
    map.insert(TOOL_ARGUMENTS_KEY, m_toolArguments);
    return map;
}

bool CMakeBuildStep::fromMap(const QVariantMap &map)
{
    const QStringList targetList = map.value(BUILD_TARGETS_KEY).toStringList();
    if (!targetList.isEmpty())
        m_buildTarget = targetList.last();
    m_cmakeArguments = map.value(CMAKE_ARGUMENTS_KEY).toString();
    m_toolArguments = map.value(TOOL_ARGUMENTS_KEY).toString();
    if (map.value(ADD_RUNCONFIGURATION_ARGUMENT_KEY, false).toBool())
        m_buildTarget = ADD_RUNCONFIGURATION_TEXT;

    return BuildStep::fromMap(map);
}

bool CMakeBuildStep::init()
{
    bool canInit = true;
    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();
    QTC_ASSERT(bc, return false);
    if (!bc->isEnabled()) {
        emit addTask(BuildSystemTask(Task::Error,
                                     tr("The build configuration is currently disabled.")));
        canInit = false;
    }

    CMakeTool *tool = CMakeKitAspect::cmakeTool(target()->kit());
    if (!tool || !tool->isValid()) {
        emit addTask(BuildSystemTask(Task::Error,
                          tr("A CMake tool must be set up for building. "
                             "Configure a CMake tool in the kit options.")));
        canInit = false;
    }

    RunConfiguration *rc =  target()->activeRunConfiguration();
    if (isCurrentExecutableTarget(m_buildTarget) && (!rc || rc->buildKey().isEmpty())) {
        emit addTask(BuildSystemTask(Task::Error,
                          QCoreApplication::translate("ProjectExplorer::Task",
                                    "You asked to build the current Run Configuration's build target only, "
                                    "but it is not associated with a build target. "
                                    "Update the Make Step in your build settings.")));
        canInit = false;
    }

    if (!canInit) {
        emitFaultyConfigurationMessage();
        return false;
    }

    // Warn if doing out-of-source builds with a CMakeCache.txt is the source directory
    const Utils::FilePath projectDirectory = bc->target()->project()->projectDirectory();
    if (bc->buildDirectory() != projectDirectory) {
        if (projectDirectory.pathAppended("CMakeCache.txt").exists()) {
            emit addTask(BuildSystemTask(Task::Warning,
                              tr("There is a CMakeCache.txt file in \"%1\", which suggest an "
                                 "in-source build was done before. You are now building in \"%2\", "
                                 "and the CMakeCache.txt file might confuse CMake.")
                              .arg(projectDirectory.toUserOutput(), bc->buildDirectory().toUserOutput())));
        }
    }

    setIgnoreReturnValue(m_buildTarget == CMakeBuildStep::cleanTarget());

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    if (!env.expandedValueForKey("NINJA_STATUS").startsWith(m_ninjaProgressString))
        env.set("NINJA_STATUS", m_ninjaProgressString + "%o/sec] ");
    pp->setEnvironment(env);
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommandLine(cmakeCommand(rc));
    pp->resolveAll();

    return AbstractProcessStep::init();
}

void CMakeBuildStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    CMakeParser *cmakeParser = new CMakeParser;
    cmakeParser->setSourceDirectory(project()->projectDirectory().toString());
    formatter->addLineParsers({cmakeParser, new GnuMakeParser});
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

void CMakeBuildStep::doRun()
{
    // Make sure CMake state was written to disk before trying to build:
    m_waiting = false;
    auto bs = static_cast<CMakeBuildSystem *>(buildSystem());
    if (bs->persistCMakeState()) {
        emit addOutput(tr("Persisting CMake state..."), BuildStep::OutputFormat::NormalMessage);
        m_waiting = true;
    } else if (buildSystem()->isWaitingForParse()) {
        emit addOutput(tr("Running CMake in preparation to build..."), BuildStep::OutputFormat::NormalMessage);
        m_waiting = true;
    }

    if (m_waiting) {
        m_runTrigger = connect(target(), &Target::parsingFinished,
                               this, [this](bool success) { handleProjectWasParsed(success); });
    } else {
        runImpl();
    }
}

void CMakeBuildStep::runImpl()
{
    // Do the actual build:
    AbstractProcessStep::doRun();
}

void CMakeBuildStep::handleProjectWasParsed(bool success)
{
    m_waiting = false;
    disconnect(m_runTrigger);
    if (isCanceled()) {
        emit finished(false);
    } else if (success) {
        runImpl();
    } else {
        AbstractProcessStep::stdError(tr("Project did not parse successfully, cannot build."));
        emit finished(false);
    }
}

BuildStepConfigWidget *CMakeBuildStep::createConfigWidget()
{
    return new CMakeBuildStepConfigWidget(this);
}

QString CMakeBuildStep::defaultBuildTarget() const
{
    const BuildStepList *const bsl = stepList();
    QTC_ASSERT(bsl, return {});
    const Core::Id parentId = bsl->id();
    if (parentId == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        return cleanTarget();
    if (parentId == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return installTarget();
    return allTarget();
}

void CMakeBuildStep::stdOutput(const QString &output)
{
    int offset = 0;
    while (offset != -1) {
        const int newlinePos = output.indexOf('\n', offset);
        QString line;
        if (newlinePos == -1) {
            line = output.mid(offset);
            offset = -1;
        } else {
            line = output.mid(offset, newlinePos - offset + 1);
            offset = newlinePos + 1;
        }
        if (m_percentProgress.indexIn(line) != -1) {
            AbstractProcessStep::stdOutput(line);
            bool ok = false;
            int percent = m_percentProgress.cap(1).toInt(&ok);
            if (ok)
                emit progress(percent, QString());
            continue;
        } else if (m_ninjaProgress.indexIn(line) != -1) {
            AbstractProcessStep::stdOutput(line);
            m_useNinja = true;
            bool ok = false;
            int done = m_ninjaProgress.cap(1).toInt(&ok);
            if (ok) {
                int all = m_ninjaProgress.cap(2).toInt(&ok);
                if (ok && all != 0) {
                    const int percent = static_cast<int>(100.0 * done/all);
                    emit progress(percent, QString());
                }
            }
            continue;
        }
        if (m_useNinja)
            AbstractProcessStep::stdError(line);
        else
            AbstractProcessStep::stdOutput(line);
    }
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

QString CMakeBuildStep::cmakeArguments() const
{
    return m_cmakeArguments;
}

void CMakeBuildStep::setCMakeArguments(const QString &list)
{
    m_cmakeArguments = list;
}

QString CMakeBuildStep::toolArguments() const
{
    return m_toolArguments;
}

void CMakeBuildStep::setToolArguments(const QString &list)
{
    m_toolArguments = list;
}

Utils::CommandLine CMakeBuildStep::cmakeCommand(RunConfiguration *rc) const
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(target()->kit());

    Utils::CommandLine cmd(tool ? tool->cmakeExecutable() : Utils::FilePath(), {});
    cmd.addArgs({"--build", "."});

    QString target;

    if (isCurrentExecutableTarget(m_buildTarget)) {
        if (rc) {
            target = rc->buildKey();
            const int pos = target.indexOf("///::///");
            if (pos >= 0) {
                target = target.mid(pos + 8);
            }
        } else {
            target = "<i>&lt;" + tr(ADD_RUNCONFIGURATION_TEXT) + "&gt;</i>";
        }
    } else {
        target = m_buildTarget;
    }

    cmd.addArgs({"--target", target});
    cmd.addArgs(m_cmakeArguments, Utils::CommandLine::Raw);

    if (!m_toolArguments.isEmpty()) {
        cmd.addArg("--");
        cmd.addArgs(m_toolArguments, Utils::CommandLine::Raw);
    }

    return cmd;
}

QStringList CMakeBuildStep::knownBuildTargets()
{
    auto bs = qobject_cast<CMakeBuildSystem *>(buildSystem());
    return bs ? bs->buildTargetTitles() : QStringList();
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

CMakeBuildStepConfigWidget::CMakeBuildStepConfigWidget(CMakeBuildStep *buildStep)
    : BuildStepConfigWidget(buildStep)
    , m_buildStep(buildStep)
    , m_cmakeArguments(new QLineEdit)
    , m_toolArguments(new QLineEdit)
    , m_buildTargetsList(new QListWidget)
{
    setDisplayName(tr("Build", "CMakeProjectManager::CMakeBuildStepConfigWidget display name."));

    auto fl = new QFormLayout(this);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    fl->addRow(tr("CMake arguments:"), m_cmakeArguments);
    m_cmakeArguments->setText(m_buildStep->cmakeArguments());
    fl->addRow(tr("Tool arguments:"), m_toolArguments);
    m_toolArguments->setText(m_buildStep->toolArguments());

    m_buildTargetsList->setFrameStyle(QFrame::NoFrame);
    m_buildTargetsList->setMinimumHeight(200);

    auto frame = new QFrame(this);
    frame->setFrameStyle(QFrame::StyledPanel);
    auto frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_buildTargetsList,
                                                                       Core::ItemViewFind::LightColored));

    fl->addRow(tr("Targets:"), frame);

    buildTargetsChanged();
    updateDetails();

    connect(m_cmakeArguments, &QLineEdit::textEdited, this, &CMakeBuildStepConfigWidget::cmakeArgumentsEdited);
    connect(m_toolArguments, &QLineEdit::textEdited, this, &CMakeBuildStepConfigWidget::toolArgumentsEdited);
    connect(m_buildTargetsList, &QListWidget::itemChanged, this, &CMakeBuildStepConfigWidget::itemChanged);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &CMakeBuildStepConfigWidget::updateDetails);

    connect(m_buildStep,
            &CMakeBuildStep::buildTargetsChanged,
            this,
            &CMakeBuildStepConfigWidget::buildTargetsChanged);

    connect(m_buildStep,
            &CMakeBuildStep::targetToBuildChanged,
            this,
            &CMakeBuildStepConfigWidget::updateBuildTarget);

    connect(m_buildStep->buildConfiguration(),
            &BuildConfiguration::environmentChanged,
            this,
            &CMakeBuildStepConfigWidget::updateDetails);
}

void CMakeBuildStepConfigWidget::cmakeArgumentsEdited() {
    m_buildStep->setCMakeArguments(m_cmakeArguments->text());
    updateDetails();
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

void CMakeBuildStepConfigWidget::buildTargetsChanged()
{
    {
        auto addItem = [this](const QString &buildTarget,
                const QString &displayName) {
            auto item = new QListWidgetItem(m_buildTargetsList);
            auto button = new QRadioButton(displayName);
            connect(button, &QRadioButton::toggled, this, [this, buildTarget](bool toggled) {
                if (toggled) {
                    m_buildStep->setBuildTarget(buildTarget);
                }
            });
            m_buildTargetsList->setItemWidget(item, button);
            item->setData(Qt::UserRole, buildTarget);
        };

        QSignalBlocker blocker(m_buildTargetsList);
        m_buildTargetsList->clear();

        QStringList targetList = m_buildStep->knownBuildTargets();
        targetList.sort();

        QFont italics;
        italics.setItalic(true);

        addItem(ADD_RUNCONFIGURATION_TEXT, tr(ADD_RUNCONFIGURATION_TEXT));

        foreach (const QString &buildTarget, targetList)
            addItem(buildTarget, buildTarget);

        for (int i = 0; i < m_buildTargetsList->count(); ++i) {
            QListWidgetItem *item = m_buildTargetsList->item(i);
            const QString title = item->data(Qt::UserRole).toString();
            QRadioButton *radio = itemWidget(item);

            radio->setChecked(m_buildStep->buildsBuildTarget(title));

            // Print utility targets in italics:
            if (CMakeBuildStep::specialTargets().contains(title) || title == ADD_RUNCONFIGURATION_TEXT)
                radio->setFont(italics);
        }
    }
    updateDetails();
}

void CMakeBuildStepConfigWidget::updateBuildTarget()
{
    const QString buildTarget = m_buildStep->buildTarget();
    {
        QSignalBlocker blocker(m_buildTargetsList);
        for (int y = 0; y < m_buildTargetsList->count(); ++y) {
            QListWidgetItem *item = m_buildTargetsList->item(y);
            const QString itemTarget = item->data(Qt::UserRole).toString();

            if (itemTarget == buildTarget) {
                QRadioButton *radio = itemWidget(item);
                radio->setChecked(true);
            }
        }
    }
    updateDetails();
}

QRadioButton *CMakeBuildStepConfigWidget::itemWidget(QListWidgetItem *item)
{
    return static_cast<QRadioButton *>(m_buildTargetsList->itemWidget(item));
}

void CMakeBuildStepConfigWidget::updateDetails()
{
    ProcessParameters param;
    param.setMacroExpander(m_buildStep->macroExpander());
    param.setEnvironment(m_buildStep->buildEnvironment());
    param.setWorkingDirectory(m_buildStep->buildDirectory());
    param.setCommandLine(m_buildStep->cmakeCommand(nullptr));

    setSummaryText(param.summary(displayName()));
}

//
// CMakeBuildStepFactory
//

CMakeBuildStepFactory::CMakeBuildStepFactory()
{
    registerStep<CMakeBuildStep>(Constants::CMAKE_BUILD_STEP_ID);
    setDisplayName(CMakeBuildStep::tr("Build", "Display name for CMakeProjectManager::CMakeBuildStep id."));
    setSupportedProjectType(Constants::CMAKE_PROJECT_ID);
}

void CMakeBuildStep::processStarted()
{
    m_useNinja = false;
    AbstractProcessStep::processStarted();
}

void CMakeBuildStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    emit progress(100, QString());
}

} // Internal
} // CMakeProjectManager
