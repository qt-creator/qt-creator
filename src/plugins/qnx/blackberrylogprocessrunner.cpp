/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrylogprocessrunner.h"
#include "slog2inforunner.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>
#include <utils/qtcassert.h>

using namespace Qnx::Internal;

BlackBerryLogProcessRunner::BlackBerryLogProcessRunner(QObject *parent, const QString& appId, const BlackBerryDeviceConfiguration::ConstPtr &device)
    : QObject(parent)
{
    Q_ASSERT(!appId.isEmpty() && device);

    m_appId = appId;
    m_device = device;

    m_slog2InfoRunner = new Slog2InfoRunner(appId, m_device, this);
    connect(m_slog2InfoRunner, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(m_slog2InfoRunner, SIGNAL(output(QString,Utils::OutputFormat)), this, SIGNAL(output(QString,Utils::OutputFormat)));

    m_tailProcess = new ProjectExplorer::SshDeviceProcess(m_device, this);
    connect(m_tailProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(readTailStandardOutput()));
    connect(m_tailProcess, SIGNAL(readyReadStandardError()), this, SLOT(readTailStandardError()));
    connect(m_tailProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(handleTailProcessError()));
}

void BlackBerryLogProcessRunner::start()
{
    m_slog2InfoRunner->start();
    launchTailProcess();
}

void BlackBerryLogProcessRunner::readTailStandardOutput()
{
    const QString message = QString::fromLatin1(m_tailProcess->readAllStandardOutput());
    emit output(message, Utils::StdOutFormat);
}

void BlackBerryLogProcessRunner::readTailStandardError()
{
    const QString message = QString::fromLatin1(m_tailProcess->readAllStandardError());
    emit output(message, Utils::StdErrFormat);
}

void BlackBerryLogProcessRunner::handleTailProcessError()
{
    emit output(tr("Cannot show debug output. Error: %1").arg(m_tailProcess->errorString()), Utils::StdErrFormat);
}

void BlackBerryLogProcessRunner::launchTailProcess()
{
    if (m_tailProcess->state() == QProcess::Running)
        return;

    QStringList parameters;
    parameters << QLatin1String("-c")
               << QLatin1String("+1")
               << QLatin1String("-f")
               << QLatin1String("/accounts/1000/appdata/") + m_appId + QLatin1String("/logs/log");
    m_tailProcess->start(QLatin1String("tail"), parameters);
}

void BlackBerryLogProcessRunner::stop()
{
    m_slog2InfoRunner->stop();
    m_tailProcess->kill();

    emit finished();
}
