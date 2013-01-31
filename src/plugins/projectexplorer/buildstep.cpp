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

#include "buildstep.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "deployconfiguration.h"
#include "target.h"

/*!
    \class ProjectExplorer::BuildStep

    \brief BuildSteps are the primary way plugin developers can customize
    how their projects (or projects from other plugins) are build.

    Building a project, is done by taking the list of buildsteps
    from the project and calling first init() than run() on them.

    That means to change the way your project is build, reimplemnt
    this class and add your Step to the buildStep list of the project.

    Note: The projects own the buildstep, do not delete them yourself.

    init() is called in the GUI thread and can be used to query the
    project for any information you need.

    run() is run via QtConccurrent in a own thread, if you need an
    eventloop you need to create it yourself!
*/

/*!
    \fn bool ProjectExplorer::BuildStep::init()

    This function is run in the gui thread,
    use it to retrieve any information that you need in run()
*/

/*!
    \fn void ProjectExplorer::BuildStep::run(QFutureInterface<bool> &fi)

    Reimplement this. This function is called when the target is build.
    By default this function is NOT run in the gui thread. It runs in its
    own thread. If you need an event loop, you need to create one.
    This function should block until the task is done

    The absolute minimal implementation is:
    \code
    fi.reportResult(true);
    \endcode

    By returning true from \sa runInGuiThread() this function is called in the
    gui thread. Then the function should not block and instead the
    finished() signal should be emitted.
*/

/*!
    \fn BuildStepConfigWidget *ProjectExplorer::BuildStep::createConfigWidget()

    Returns the Widget shown in the target settings dialog for this buildStep;
    ownership is transferred to the caller.
*/

/*!
    \fn bool ProjectExplorer::BuildStep::immutable() const

    If this function returns true, the user can't delete this BuildStep for this target
    and the user is prevented from changing the order immutable steps are run
    the default implementation returns false.
*/

/*!
    \fn  void ProjectExplorer::BuildStep::addTask(const ProjectExplorer::Task &task)
    \brief Add a task.
*/

/*!
    \fn  void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format,
              ProjectExplorer::BuildStep::OutputNewlineSetting newlineSetting)

    The string is added to the generated output, usually in the output window.
    It should be in plain text, with the format in the parameter.
*/

/*!
    \fn void ProjectExplorer::BuildStep::cancel() const

    This function needs to be reimplemented only for BuildSteps that return false from \sa runInGuiThread.
*/

/*!
    \fn  void ProjectExplorer::BuildStep::finished()
    \brief This signal needs to be emitted if the BuildStep runs in the gui thread.
*/

static const char buildStepEnabledKey[] = "ProjectExplorer.BuildStep.Enabled";

using namespace ProjectExplorer;

BuildStep::BuildStep(BuildStepList *bsl, const Core::Id id) :
    ProjectConfiguration(bsl, id), m_enabled(true)
{
    Q_ASSERT(bsl);
}

BuildStep::BuildStep(BuildStepList *bsl, BuildStep *bs) :
    ProjectConfiguration(bsl, bs), m_enabled(bs->m_enabled)
{
    Q_ASSERT(bsl);
    setDisplayName(bs->displayName());
}

BuildStep::~BuildStep()
{
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

bool BuildStep::immutable() const
{
    return false;
}

bool BuildStep::runInGuiThread() const
{
    return false;
}

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

IBuildStepFactory::~IBuildStepFactory()
{ }
