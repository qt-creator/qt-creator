/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimbletaskstep.h"
#include "nimbletaskstepwidget.h"
#include "nimconstants.h"
#include "nimbleproject.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/fileutils.h>

#include <QStandardPaths>

using namespace Nim;
using namespace ProjectExplorer;

NimbleTaskStep::NimbleTaskStep(BuildStepList *parentList, Utils::Id id)
    : AbstractProcessStep(parentList, id)
{
    setDefaultDisplayName(tr(Constants::C_NIMBLETASKSTEP_DISPLAY));
    setDisplayName(tr(Constants::C_NIMBLETASKSTEP_DISPLAY));
}

bool NimbleTaskStep::init()
{
    processParameters()->setEnvironment(buildEnvironment());
    processParameters()->setWorkingDirectory(project()->projectDirectory());
    return validate() && AbstractProcessStep::init();
}

BuildStepConfigWidget *NimbleTaskStep::createConfigWidget()
{
    return new NimbleTaskStepWidget(this);
}

bool NimbleTaskStep::fromMap(const QVariantMap &map)
{
    setTaskName(map.value(Constants::C_NIMBLETASKSTEP_TASKNAME, QString()).toString());
    setTaskArgs(map.value(Constants::C_NIMBLETASKSTEP_TASKARGS, QString()).toString());
    return validate() ? AbstractProcessStep::fromMap(map) : false;
}

QVariantMap NimbleTaskStep::toMap() const
{
    QVariantMap result = AbstractProcessStep::toMap();
    result[Constants::C_NIMBLETASKSTEP_TASKNAME] = taskName();
    result[Constants::C_NIMBLETASKSTEP_TASKARGS] = taskArgs();
    return result;
}

void NimbleTaskStep::setTaskName(const QString &name)
{
    if (m_taskName == name)
        return;
    m_taskName = name;
    emit taskNameChanged(name);
    updateCommandLine();
}

void NimbleTaskStep::setTaskArgs(const QString &args)
{
    if (m_taskArgs == args)
        return;
    m_taskArgs = args;
    emit taskArgsChanged(args);
    updateCommandLine();
}

void NimbleTaskStep::updateCommandLine()
{
    QString args = m_taskName + " " + m_taskArgs;
    Utils::CommandLine commandLine(Utils::FilePath::fromString(QStandardPaths::findExecutable("nimble")),
                                   args, Utils::CommandLine::Raw);

    processParameters()->setCommandLine(commandLine);
}

bool NimbleTaskStep::validate()
{
    if (m_taskName.isEmpty())
        return true;

    auto nimbleBuildSystem = dynamic_cast<NimbleBuildSystem*>(buildSystem());
    QTC_ASSERT(nimbleBuildSystem, return false);

    if (!Utils::contains(nimbleBuildSystem->tasks(), [this](const NimbleTask &task){ return task.name == m_taskName; })) {
        emit addTask(BuildSystemTask(Task::Error, tr("Nimble task %1 not found.").arg(m_taskName)));
        emitFaultyConfigurationMessage();
        return false;
    }

    return true;
}

NimbleTaskStepFactory::NimbleTaskStepFactory()
{
    registerStep<NimbleTaskStep>(Constants::C_NIMBLETASKSTEP_ID);
    setDisplayName(NimbleTaskStep::tr("Nimble Task"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN,
                           ProjectExplorer::Constants::BUILDSTEPS_DEPLOY});
    setSupportedConfiguration(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setRepeatable(true);
}
