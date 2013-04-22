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

#include "blackberrydeviceconnection.h"

#include "blackberryconfiguration.h"
#include "qnxutils.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <utils/environment.h>

#include <QProcess>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char CONNECT_SUCCESS_MSG[] = "Successfully connected";
}

BlackBerryDeviceConnection::BlackBerryDeviceConnection() :
    QObject(),
    m_connectionState(Disconnected),
    m_process(new QProcess(this))
{
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished()));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processFinished()));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
}

void BlackBerryDeviceConnection::connectDevice(const ProjectExplorer::IDevice::ConstPtr &device)
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    QnxUtils::prependQnxMapToEnvironment(BlackBerryConfiguration::instance().qnxEnv(), env);
    m_process->setEnvironment(env.toStringList());

    m_host = device->sshParameters().host;
    const QString password = device->sshParameters().password;
    const QString publicKeyFile = device->sshParameters().privateKeyFile + QLatin1String(".pub");

    // Since killing the blackberry-connect script won't kill the java process it launches,
    // let's just call the java process directly instead.
    QString command = env.searchInPath(QLatin1String("java"));
    if (command.isEmpty()) {
        const QString line = tr("Error connecting to device: java could not be found in the environment.") + QLatin1Char('\n');
        emit processOutput(line);
        m_messageLog.append(line);
        return;
    }

    QStringList args;
    args << QLatin1String("-Xmx512M");
    args << QLatin1String("-jar");
    args << env.value(QLatin1String("QNX_HOST")) + QLatin1String("/usr/lib/Connect.jar");

    args << QLatin1String("-targetHost") << m_host;
    if (!password.isEmpty())
        args << QLatin1String("-password") << password;
    args << QLatin1String("-sshPublicKey") << publicKeyFile;

    m_connectionState = Connecting;
    m_process->start(command, args);
    m_messageLog.clear();
    emit deviceAboutToConnect();
}

void BlackBerryDeviceConnection::disconnectDevice()
{
    m_process->terminate();
    if (!m_process->waitForFinished(5000))
        m_process->kill();
    m_connectionState = Disconnected;
}

QString BlackBerryDeviceConnection::host() const
{
    return m_host;
}

BlackBerryDeviceConnection::State BlackBerryDeviceConnection::connectionState()
{
    return m_connectionState;
}

QString BlackBerryDeviceConnection::messageLog() const
{
    return m_messageLog;
}

void BlackBerryDeviceConnection::processFinished()
{
    m_connectionState = Disconnected;
    emit deviceDisconnected();
}

void BlackBerryDeviceConnection::readStandardOutput()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(m_process->readLine());

        emit processOutput(line);
        m_messageLog.append(line);

        if (line.contains(QLatin1String(CONNECT_SUCCESS_MSG))) {
            m_connectionState = Connected;
            emit deviceConnected();
        }
    }
}

void BlackBerryDeviceConnection::readStandardError()
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(m_process->readLine());

        emit processOutput(line);
        m_messageLog.append(line);
    }
}
