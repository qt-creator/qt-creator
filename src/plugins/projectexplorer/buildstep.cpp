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
#include "deployconfiguration.h"
#include "target.h"

#include <utils/algorithm.h>

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

static const char buildStepEnabledKey[] = "ProjectExplorer.BuildStep.Enabled";

using namespace ProjectExplorer;

BuildStep::BuildStep(BuildStepList *bsl, Core::Id id) :
    ProjectConfiguration(bsl, id), m_enabled(true)
{
    Q_ASSERT(bsl);
    ctor();
}

BuildStep::BuildStep(BuildStepList *bsl, BuildStep *bs) :
    ProjectConfiguration(bsl, bs), m_enabled(bs->m_enabled)
{
    Q_ASSERT(bsl);
    setDisplayName(bs->displayName());
    ctor();
}

void BuildStep::ctor()
{
    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Build Step"));
    expander->setAccumulating(true);
    expander->registerSubProvider([this] { return projectConfiguration()->macroExpander(); });
}

bool BuildStep::fromMap(const QVariantMap &map)
{
    m_enabled = map.value(QLatin1String(buildStepEnabledKey), true).toBool();
    return ProjectConfiguration::fromMap(map);
}

QVariantMap BuildStep::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();
    map.insert(QLatin1String(buildStepEnabledKey), m_enabled);
    return map;
}

BuildConfiguration *BuildStep::buildConfiguration() const
{
    return qobject_cast<BuildConfiguration *>(parent()->parent());
}

DeployConfiguration *BuildStep::deployConfiguration() const
{
    return qobject_cast<DeployConfiguration *>(parent()->parent());
}

ProjectConfiguration *BuildStep::projectConfiguration() const
{
    return static_cast<ProjectConfiguration *>(parent()->parent());
}

Target *BuildStep::target() const
{
    return qobject_cast<Target *>(parent()->parent()->parent());
}

Project *BuildStep::project() const
{
    return target()->project();
}

void BuildStep::reportRunResult(QFutureInterface<bool> &fi, bool success)
{
    fi.reportResult(success);
    fi.reportFinished();
}

/*!
    If this function returns \c true, the user cannot delete this build step for
    this target and the user is prevented from changing the order in which
    immutable steps are run. The default implementation returns \c false.
*/

bool BuildStep::immutable() const
{
    return false;
}

bool BuildStep::runInGuiThread() const
{
    return false;
}

/*!
    This function needs to be reimplemented only for build steps that return
    \c false from runInGuiThread().

    \sa runInGuiThread()
*/
void BuildStep::cancel()
{
    // Do nothing
}

void BuildStep::setEnabled(bool b)
{
    if (m_enabled == b)
        return;
    m_enabled = b;
    emit enabledChanged();
}

bool BuildStep::enabled() const
{
    return m_enabled;
}

IBuildStepFactory::IBuildStepFactory(QObject *parent) :
    QObject(parent)
{ }

BuildStep *IBuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    const Core::Id id = idFromMap(map);
    BuildStep *bs = create(parent, id);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return nullptr;
}

