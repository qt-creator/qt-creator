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

#include "buildstep.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "customparser.h"
#include "deployconfiguration.h"
#include "kitinformation.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "target.h"

#include <coreplugin/variablechooser.h>

#include <utils/algorithm.h>
#include <utils/fileinprojectfinder.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QFormLayout>
#include <QFutureWatcher>
#include <QPointer>

/*!
    \class ProjectExplorer::BuildStep

    \brief The BuildStep class provides build steps for projects.

    Build steps are the primary way plugin developers can customize
    how their projects (or projects from other plugins) are built.

    Projects are built by taking the list of build steps
    from the project and calling first \c init() and then \c run() on them.

    To change the way your project is built, reimplement
    this class and add your build step to the build step list of the project.

    \note The projects own the build step. Do not delete them yourself.

    \c init() is called in the GUI thread and can be used to query the
    project for any information you need.

    \c run() is run via Utils::runAsync in a separate thread. If you need an
    event loop, you need to create it yourself.
*/

/*!
    \fn bool ProjectExplorer::BuildStep::init()

    This function is run in the GUI thread. Use it to retrieve any information
    that you need in the run() function.
*/

/*!
    \fn void ProjectExplorer::BuildStep::run(QFutureInterface<bool> &fi)

    Reimplement this function. It is called when the target is built.
    By default, this function is NOT run in the GUI thread, but runs in its
    own thread. If you need an event loop, you need to create one.
    This function should block until the task is done

    The absolute minimal implementation is:
    \code
    fi.reportResult(true);
    \endcode

    By returning \c true from runInGuiThread(), this function is called in
    the GUI thread. Then the function should not block and instead the
    finished() signal should be emitted.

    \sa runInGuiThread()
*/

/*!
    \fn BuildStepConfigWidget *ProjectExplorer::BuildStep::createConfigWidget()

    Returns the Widget shown in the target settings dialog for this build step.
    Ownership is transferred to the caller.
*/

/*!
    \fn  void ProjectExplorer::BuildStep::addTask(const ProjectExplorer::Task &task)
    Adds \a task.
*/

/*!
    \fn  void ProjectExplorer::BuildStep::addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format,
              ProjectExplorer::BuildStep::OutputNewlineSetting newlineSetting = DoAppendNewline) const

    The \a string is added to the generated output, usually in the output pane.
    It should be in plain text, with the format in the parameter.
*/

/*!
    \fn  void ProjectExplorer::BuildStep::finished()
    This signal needs to be emitted if the build step runs in the GUI thread.
*/

using namespace Utils;

static const char buildStepEnabledKey[] = "ProjectExplorer.BuildStep.Enabled";

namespace ProjectExplorer {

static QList<BuildStepFactory *> g_buildStepFactories;

BuildStep::BuildStep(BuildStepList *bsl, Utils::Id id) :
    ProjectConfiguration(bsl, id)
{
    QTC_CHECK(bsl->target() && bsl->target() == this->target());
}

BuildStep::~BuildStep()
{
    emit finished(false);
}

void BuildStep::run()
{
    m_cancelFlag = false;
    doRun();
}

void BuildStep::cancel()
{
    m_cancelFlag = true;
    doCancel();
}

BuildStepConfigWidget *BuildStep::createConfigWidget()
{
    auto widget = new BuildStepConfigWidget(this);

    {
        LayoutBuilder builder(widget);
        for (ProjectConfigurationAspect *aspect : m_aspects) {
            if (aspect->isVisible())
                aspect->addToLayout(builder.startNewRow());
        }
    }

    connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged,
            widget, &BuildStepConfigWidget::recreateSummary);

    widget->setSummaryUpdater(m_summaryUpdater);

    if (m_addMacroExpander)
        Core::VariableChooser::addSupportForChildWidgets(widget, macroExpander());

    return widget;
}

bool BuildStep::fromMap(const QVariantMap &map)
{
    m_enabled = map.value(buildStepEnabledKey, true).toBool();
    return ProjectConfiguration::fromMap(map);
}

QVariantMap BuildStep::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();
    map.insert(buildStepEnabledKey, m_enabled);
    return map;
}

BuildConfiguration *BuildStep::buildConfiguration() const
{
    auto config = qobject_cast<BuildConfiguration *>(parent()->parent());
    if (config)
        return config;

    // This situation should be avoided, as the step returned below is almost
    // always not the right one, but the fallback is best we can do.
    // A potential currently still valid path is accessing a build configuration
    // from a BuildStep in a DeployConfiguration. Let's hunt those down and
    // replace with explicit code there.
    QTC_CHECK(false);
    // step is not part of a build configuration, use active build configuration of step's target
    return target()->activeBuildConfiguration();
}

DeployConfiguration *BuildStep::deployConfiguration() const
{
    auto config = qobject_cast<DeployConfiguration *>(parent()->parent());
    if (config)
        return config;
    // See comment in buildConfiguration()
    QTC_CHECK(false);
    // step is not part of a deploy configuration, use active deploy configuration of step's target
    return target()->activeDeployConfiguration();
}

ProjectConfiguration *BuildStep::projectConfiguration() const
{
    return static_cast<ProjectConfiguration *>(parent()->parent());
}

BuildSystem *BuildStep::buildSystem() const
{
    if (auto bc = buildConfiguration())
        return bc->buildSystem();
    return target()->buildSystem();
}

Environment BuildStep::buildEnvironment() const
{
    if (auto bc = buildConfiguration())
        return bc->environment();
    return Environment::systemEnvironment();
}

FilePath BuildStep::buildDirectory() const
{
    if (auto bc = buildConfiguration())
        return bc->buildDirectory();
    return {};
}

BuildConfiguration::BuildType BuildStep::buildType() const
{
    if (auto bc = buildConfiguration())
        return bc->buildType();
    return BuildConfiguration::Unknown;
}

Utils::MacroExpander *BuildStep::macroExpander() const
{
    if (auto bc = buildConfiguration())
        return bc->macroExpander();
    return Utils::globalMacroExpander();
}

QString BuildStep::fallbackWorkingDirectory() const
{
    if (buildConfiguration())
        return {Constants::DEFAULT_WORKING_DIR};
    return {Constants::DEFAULT_WORKING_DIR_ALTERNATE};
}

void BuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    for (const Utils::Id id : buildConfiguration()->customParsers()) {
        if (Internal::CustomParser * const parser = Internal::CustomParser::createFromId(id))
            formatter->addLineParser(parser);
    }
    Utils::FileInProjectFinder fileFinder;
    fileFinder.setProjectDirectory(project()->projectDirectory());
    fileFinder.setProjectFiles(project()->files(Project::AllFiles));
    formatter->setFileFinder(fileFinder);
}

void BuildStep::reportRunResult(QFutureInterface<bool> &fi, bool success)
{
    fi.reportResult(success);
    fi.reportFinished();
}

bool BuildStep::widgetExpandedByDefault() const
{
    return m_widgetExpandedByDefault;
}

void BuildStep::setWidgetExpandedByDefault(bool widgetExpandedByDefault)
{
    m_widgetExpandedByDefault = widgetExpandedByDefault;
}

QVariant BuildStep::data(Utils::Id id) const
{
    Q_UNUSED(id)
    return {};
}

/*!
  \fn BuildStep::isImmutable()

    If this function returns \c true, the user cannot delete this build step for
    this target and the user is prevented from changing the order in which
    immutable steps are run. The default implementation returns \c false.
*/

void BuildStep::runInThread(const std::function<bool()> &syncImpl)
{
    m_runInGuiThread = false;
    m_cancelFlag = false;
    auto * const watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher] {
        emit finished(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(Utils::runAsync(syncImpl));
}

std::function<bool ()> BuildStep::cancelChecker() const
{
    return [step = QPointer<const BuildStep>(this)] { return step && step->isCanceled(); };
}

bool BuildStep::isCanceled() const
{
    return m_cancelFlag;
}

void BuildStep::doCancel()
{
    QTC_ASSERT(!m_runInGuiThread, qWarning() << "Build step" << displayName()
               << "neeeds to implement the doCancel() function");
}

void BuildStep::addMacroExpander()
{
    m_addMacroExpander = true;
}

void BuildStep::setSummaryUpdater(const std::function<QString ()> &summaryUpdater)
{
    m_summaryUpdater = summaryUpdater;
}

void BuildStep::setEnabled(bool b)
{
    if (m_enabled == b)
        return;
    m_enabled = b;
    emit enabledChanged();
}

BuildStepList *BuildStep::stepList() const
{
    return qobject_cast<BuildStepList *>(parent());
}

bool BuildStep::enabled() const
{
    return m_enabled;
}

BuildStepFactory::BuildStepFactory()
{
    g_buildStepFactories.append(this);
}

BuildStepFactory::~BuildStepFactory()
{
    g_buildStepFactories.removeOne(this);
}

const QList<BuildStepFactory *> BuildStepFactory::allBuildStepFactories()
{
    return g_buildStepFactories;
}

bool BuildStepFactory::canHandle(BuildStepList *bsl) const
{
    if (!m_supportedStepLists.isEmpty() && !m_supportedStepLists.contains(bsl->id()))
        return false;

    auto config = qobject_cast<ProjectConfiguration *>(bsl->parent());

    if (!m_supportedDeviceTypes.isEmpty()) {
        Target *target = bsl->target();
        QTC_ASSERT(target, return false);
        Utils::Id deviceType = DeviceTypeKitAspect::deviceTypeId(target->kit());
        if (!m_supportedDeviceTypes.contains(deviceType))
            return false;
    }

    if (m_supportedProjectType.isValid()) {
        if (!config)
            return false;
        Utils::Id projectId = config->project()->id();
        if (projectId != m_supportedProjectType)
            return false;
    }

    if (!m_isRepeatable && bsl->contains(m_info.id))
        return false;

    if (m_supportedConfiguration.isValid()) {
        if (!config)
            return false;
        Utils::Id configId = config->id();
        if (configId != m_supportedConfiguration)
            return false;
    }

    return true;
}

void BuildStepFactory::setDisplayName(const QString &displayName)
{
    m_info.displayName = displayName;
}

void BuildStepFactory::setFlags(BuildStepInfo::Flags flags)
{
    m_info.flags = flags;
}

void BuildStepFactory::setSupportedStepList(Utils::Id id)
{
    m_supportedStepLists = {id};
}

void BuildStepFactory::setSupportedStepLists(const QList<Utils::Id> &ids)
{
    m_supportedStepLists = ids;
}

void BuildStepFactory::setSupportedConfiguration(Utils::Id id)
{
    m_supportedConfiguration = id;
}

void BuildStepFactory::setSupportedProjectType(Utils::Id id)
{
    m_supportedProjectType = id;
}

void BuildStepFactory::setSupportedDeviceType(Utils::Id id)
{
    m_supportedDeviceTypes = {id};
}

void BuildStepFactory::setSupportedDeviceTypes(const QList<Utils::Id> &ids)
{
    m_supportedDeviceTypes = ids;
}

BuildStepInfo BuildStepFactory::stepInfo() const
{
    return m_info;
}

Utils::Id BuildStepFactory::stepId() const
{
    return m_info.id;
}

BuildStep *BuildStepFactory::create(BuildStepList *parent, Utils::Id id)
{
    BuildStep *bs = nullptr;
    if (id == m_info.id)
        bs = m_info.creator(parent);
    return bs;
}

BuildStep *BuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    BuildStep *bs = m_info.creator(parent);
    if (!bs)
        return nullptr;
    if (!bs->fromMap(map)) {
        QTC_CHECK(false);
        delete bs;
        return nullptr;
    }
    return bs;
}

// BuildStepConfigWidget

BuildStepConfigWidget::BuildStepConfigWidget(BuildStep *step)
    : m_step(step)
{
    m_displayName = step->displayName();
    m_summaryText = "<b>" + m_displayName + "</b>";
    connect(m_step, &ProjectConfiguration::displayNameChanged,
            this, &BuildStepConfigWidget::updateSummary);
    for (auto aspect : step->aspects()) {
        connect(aspect, &ProjectConfigurationAspect::changed,
                this, &BuildStepConfigWidget::recreateSummary);
    }
}

QString BuildStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

QString BuildStepConfigWidget::displayName() const
{
    return m_displayName;
}

void BuildStepConfigWidget::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void BuildStepConfigWidget::setSummaryText(const QString &summaryText)
{
    if (summaryText != m_summaryText) {
        m_summaryText = summaryText;
        emit updateSummary();
    }
}

void BuildStepConfigWidget::setSummaryUpdater(const std::function<QString()> &summaryUpdater)
{
    m_summaryUpdater = summaryUpdater;
    recreateSummary();
}

void BuildStepConfigWidget::recreateSummary()
{
    if (m_summaryUpdater)
        setSummaryText(m_summaryUpdater());
}

} // ProjectExplorer
