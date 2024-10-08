// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildstep.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "customparser.h"
#include "deployconfiguration.h"
#include "kitaspects.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "sanitizerparser.h"
#include "target.h"

#include <utils/fileinprojectfinder.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/variablechooser.h>

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
*/

/*!
    \fn bool ProjectExplorer::BuildStep::init()

    This function is run in the GUI thread. Use it to retrieve any information
    that you need in the run() function.
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

    The \a string is added to the generated output, usually in the output.
    It should be in plain text, with the format in the parameter.
*/

/*!
    \fn bool ProjectExplorer::BuildStep::isImmutable()

    If this function returns \c true, the user cannot delete this build step for
    this target and the user is prevented from changing the order in which
    immutable steps are run. The default implementation returns \c false.
*/

using namespace Utils;

static const char buildStepEnabledKey[] = "ProjectExplorer.BuildStep.Enabled";

namespace ProjectExplorer {

static QList<BuildStepFactory *> g_buildStepFactories;

BuildStep::BuildStep(BuildStepList *bsl, Id id)
    : ProjectConfiguration(bsl->target(), id)
    , m_stepList(bsl)
{
    if (auto bc = buildConfiguration())
        setMacroExpander(bc->macroExpander());

    connect(this, &ProjectConfiguration::displayNameChanged, this, &BuildStep::updateSummary);
}

QWidget *BuildStep::doCreateConfigWidget()
{
    QWidget *widget = createConfigWidget();

    const auto recreateSummary = [this] {
        if (m_summaryUpdater)
            setSummaryText(m_summaryUpdater());
    };

    for (BaseAspect *aspect : std::as_const(*this))
        connect(aspect, &BaseAspect::changed, widget, recreateSummary);

    if (buildConfiguration()) {
        connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged,
                widget, recreateSummary);
    }

    recreateSummary();

    return widget;
}

QWidget *BuildStep::createConfigWidget()
{
    Layouting::Form form;
    form.setNoMargins();
    for (BaseAspect *aspect : std::as_const(*this)) {
        if (aspect->isVisible()) {
            form.addItem(aspect);
            form.flush();
        }
    }

    return form.emerge();
}

void BuildStep::fromMap(const Store &map)
{
    m_stepEnabled = map.value(buildStepEnabledKey, true).toBool();
    ProjectConfiguration::fromMap(map);
}

void BuildStep::toMap(Store &map) const
{
    ProjectConfiguration::toMap(map);
    map.insert(buildStepEnabledKey, m_stepEnabled);
}

BuildConfiguration *BuildStep::buildConfiguration() const
{
    auto config = qobject_cast<BuildConfiguration *>(projectConfiguration());
    if (config)
        return config;

    // step is not part of a build configuration, use active build configuration of step's target
    return target()->activeBuildConfiguration();
}

DeployConfiguration *BuildStep::deployConfiguration() const
{
    auto config = qobject_cast<DeployConfiguration *>(projectConfiguration());
    if (config)
        return config;
    // See comment in buildConfiguration()
    QTC_CHECK(false);
    // step is not part of a deploy configuration, use active deploy configuration of step's target
    return target()->activeDeployConfiguration();
}

ProjectConfiguration *BuildStep::projectConfiguration() const
{
    return stepList()->projectConfiguration();
}

BuildSystem *BuildStep::buildSystem() const
{
    if (auto bc = buildConfiguration())
        return bc->buildSystem();
    return target()->buildSystem();
}

Environment BuildStep::buildEnvironment() const
{
    if (const auto bc = qobject_cast<BuildConfiguration *>(projectConfiguration()))
        return bc->environment();
    if (const auto bc = target()->activeBuildConfiguration())
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

QString BuildStep::fallbackWorkingDirectory() const
{
    if (buildConfiguration())
        return {Constants::DEFAULT_WORKING_DIR};
    return {Constants::DEFAULT_WORKING_DIR_ALTERNATE};
}

void BuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    if (auto bc = qobject_cast<BuildConfiguration *>(projectConfiguration())) {
        for (const Id id : bc->customParsers()) {
            if (auto parser = createCustomParserFromId(id))
                formatter->addLineParser(parser);
        }

        formatter->addLineParser(Internal::createSanitizerOutputParser());
        formatter->setForwardStdOutToStdError(buildConfiguration()->parseStdOut());
    }
    FileInProjectFinder fileFinder;
    fileFinder.setProjectDirectory(project()->projectDirectory());
    fileFinder.setProjectFiles(project()->files(Project::AllFiles));
    formatter->setFileFinder(fileFinder);
}

bool BuildStep::widgetExpandedByDefault() const
{
    return m_widgetExpandedByDefault;
}

void BuildStep::setWidgetExpandedByDefault(bool widgetExpandedByDefault)
{
    m_widgetExpandedByDefault = widgetExpandedByDefault;
}

QVariant BuildStep::data(Id id) const
{
    Q_UNUSED(id)
    return {};
}

void BuildStep::setStepEnabled(bool b)
{
    if (m_stepEnabled == b)
        return;
    m_stepEnabled = b;
    emit stepEnabledChanged();
}

BuildStepList *BuildStep::stepList() const
{
    return m_stepList;
}

bool BuildStep::stepEnabled() const
{
    return m_stepEnabled;
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

    ProjectConfiguration *config = bsl->projectConfiguration();

    if (!m_supportedDeviceTypes.isEmpty()) {
        Target *target = bsl->target();
        QTC_ASSERT(target, return false);
        Id deviceType = DeviceTypeKitAspect::deviceTypeId(target->kit());
        if (!m_supportedDeviceTypes.contains(deviceType))
            return false;
    }

    if (m_supportedProjectType.isValid()) {
        if (!config)
            return false;
        Id projectId = config->project()->id();
        if (projectId != m_supportedProjectType)
            return false;
    }

    if (!m_isRepeatable && bsl->contains(m_stepId))
        return false;

    if (m_supportedConfiguration.isValid()) {
        if (!config)
            return false;
        Id configId = config->id();
        if (configId != m_supportedConfiguration)
            return false;
    }

    return true;
}

QString BuildStepFactory::displayName() const
{
    return m_displayName;
}

void BuildStepFactory::cloneStepCreator(Id exitstingStepId, Id overrideNewStepId)
{
    m_stepId = {};
    m_creator = {};
    for (BuildStepFactory *factory : BuildStepFactory::allBuildStepFactories()) {
        if (factory->m_stepId == exitstingStepId) {
            m_creator = factory->m_creator;
            m_stepId = factory->m_stepId;
            m_displayName = factory->m_displayName;
            // Other bits are intentionally not copied as they are unlikely to be
            // useful in the cloner's context. The cloner can/has to finish the
            // setup on its own.
            break;
        }
    }
    // Existence should be guaranteed by plugin dependencies. In case it fails,
    // bark and keep the factory in a state where the invalid m_stepId keeps it
    // inaction.
    QTC_ASSERT(m_creator, return);
    if (overrideNewStepId.isValid())
        m_stepId = overrideNewStepId;
}

void BuildStepFactory::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void BuildStepFactory::setFlags(BuildStep::Flags flags)
{
    m_flags = flags;
}

void BuildStepFactory::setExtraInit(const std::function<void (BuildStep *)> &extraInit)
{
    m_extraInit = extraInit;
}

void BuildStepFactory::setSupportedStepList(Id id)
{
    m_supportedStepLists = {id};
}

void BuildStepFactory::setSupportedStepLists(const QList<Id> &ids)
{
    m_supportedStepLists = ids;
}

void BuildStepFactory::setSupportedConfiguration(Id id)
{
    m_supportedConfiguration = id;
}

void BuildStepFactory::setSupportedProjectType(Id id)
{
    m_supportedProjectType = id;
}

void BuildStepFactory::setSupportedDeviceType(Id id)
{
    m_supportedDeviceTypes = {id};
}

void BuildStepFactory::setSupportedDeviceTypes(const QList<Id> &ids)
{
    m_supportedDeviceTypes = ids;
}

BuildStep::Flags BuildStepFactory::stepFlags() const
{
    return m_flags;
}

Id BuildStepFactory::stepId() const
{
    return m_stepId;
}

BuildStep *BuildStepFactory::create(BuildStepList *parent)
{
    QTC_ASSERT(m_creator, return nullptr);
    BuildStep *step = m_creator(this, parent);
    step->setDefaultDisplayName(m_displayName);
    return step;
}

BuildStep *BuildStepFactory::restore(BuildStepList *parent, const Store &map)
{
    BuildStep *bs = create(parent);
    if (!bs)
        return nullptr;
    bs->fromMap(map);
    if (bs->hasError()) {
        QTC_CHECK(false);
        delete bs;
        return nullptr;
    }
    return bs;
}

QString BuildStep::summaryText() const
{
    if (m_summaryText.isEmpty())
        return QString("<b>%1</b>").arg(displayName());

    return m_summaryText;
}

void BuildStep::setSummaryText(const QString &summaryText)
{
    if (summaryText != m_summaryText) {
        m_summaryText = summaryText;
        emit updateSummary();
    }
}

void BuildStep::setSummaryUpdater(const std::function<QString()> &summaryUpdater)
{
    m_summaryUpdater = summaryUpdater;
}

} // ProjectExplorer
