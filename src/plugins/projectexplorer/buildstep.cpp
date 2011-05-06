/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
    This function is NOT run in the gui thread. It runs in its own thread
    If you need an event loop, you need to create one.

    The absolute minimal implementation is:
    \code
    fi.reportResult(true);
    \endcode
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

using namespace ProjectExplorer;

BuildStep::BuildStep(BuildStepList *bsl, const QString &id) :
    ProjectConfiguration(bsl, id)
{
    Q_ASSERT(bsl);
}

BuildStep::BuildStep(BuildStepList *bsl, BuildStep *bs) :
    ProjectConfiguration(bsl, bs)
{
    Q_ASSERT(bsl);
}

BuildStep::~BuildStep()
{
}

BuildConfiguration *BuildStep::buildConfiguration() const
{
    BuildConfiguration *bc = qobject_cast<BuildConfiguration *>(parent()->parent());
    if (!bc)
        bc = target()->activeBuildConfiguration();
    return bc;
}

DeployConfiguration *BuildStep::deployConfiguration() const
{
    DeployConfiguration *dc = qobject_cast<DeployConfiguration *>(parent()->parent());
    if (!dc)
        dc = target()->activeDeployConfiguration();
    return dc;
}

Target *BuildStep::target() const
{
    return qobject_cast<Target *>(parent()->parent()->parent());
}

bool BuildStep::immutable() const
{
    return false;
}

IBuildStepFactory::IBuildStepFactory(QObject *parent) :
    QObject(parent)
{ }

IBuildStepFactory::~IBuildStepFactory()
{ }
