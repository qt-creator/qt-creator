/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryruncontrol.h"
#include "blackberryapplicationrunner.h"
#include "blackberryrunconfiguration.h"
#include "blackberrydeviceconnectionmanager.h"

#include <projectexplorer/target.h>

#include <QIcon>
#include <QTimer>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryRunControl::BlackBerryRunControl(BlackBerryRunConfiguration *runConfiguration)
    : ProjectExplorer::RunControl(runConfiguration, ProjectExplorer::NormalRunMode)
{
    m_device = BlackBerryDeviceConfiguration::device(runConfiguration->target()->kit());
    m_runner = new BlackBerryApplicationRunner(false, runConfiguration, this);

    connect(m_runner, SIGNAL(started()), this, SIGNAL(started()));
    connect(m_runner, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(m_runner, SIGNAL(output(QString,Utils::OutputFormat)),
            this, SLOT(appendMessage(QString,Utils::OutputFormat)));
    connect(m_runner, SIGNAL(startFailed(QString)), this, SLOT(handleStartFailed(QString)));
}

void BlackBerryRunControl::start()
{
    checkDeviceConnection();
}

ProjectExplorer::RunControl::StopResult BlackBerryRunControl::stop()
{
    return m_runner->stop();
}

bool BlackBerryRunControl::isRunning() const
{
    return m_runner->isRunning();
}

QIcon BlackBerryRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void BlackBerryRunControl::handleStartFailed(const QString &message)
{
    appendMessage(message, Utils::StdErrFormat);
}

void BlackBerryRunControl::handleDeviceConnected()
{
     m_runner->start();
}

void BlackBerryRunControl::displayConnectionOutput(Core::Id deviceId, const QString &output)
{
    if (deviceId != m_device->id())
        return;

    if (output.contains(QLatin1String("Info:")))
        appendMessage(output, Utils::StdOutFormat);
    else if (output.contains(QLatin1String("Error:")))
        appendMessage(output, Utils::StdErrFormat);
}

void BlackBerryRunControl::checkDeviceConnection()
{
    if (!BlackBerryDeviceConnectionManager::instance()->isConnected(m_device->id())) {
        connect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(deviceConnected()),
                this, SLOT(handleDeviceConnected()));
        connect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(connectionOutput(Core::Id,QString)),
                this, SLOT(displayConnectionOutput(Core::Id,QString)));
        BlackBerryDeviceConnectionManager::instance()->connectDevice(m_device->id());
    } else {
        m_runner->start();
    }
}
