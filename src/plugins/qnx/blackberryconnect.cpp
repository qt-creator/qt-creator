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

#include "blackberryconnect.h"
#include "blackberryrunconfiguration.h"
#include "blackberrydeployconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QProcess>
#include <QApplication>

using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char CONNECT_CMD[] = "java";
const char CONNECT_SUCCESS_MSG[] = "Successfully connected";
}

QMap<QString, BlackBerryConnect *> BlackBerryConnect::m_instances = QMap<QString, BlackBerryConnect *>();
QMap<QString, int> BlackBerryConnect::m_usageCount = QMap<QString, int>();

BlackBerryConnect *BlackBerryConnect::instance(BlackBerryRunConfiguration *runConfig)
{

    BlackBerryDeviceConfiguration::ConstPtr device
            = BlackBerryDeviceConfiguration::device(runConfig->target()->kit());
    QString deviceHost;
    if (device)
        deviceHost = device->sshParameters().host;
    if (!m_instances.contains(deviceHost)) {
        m_instances[deviceHost] = new BlackBerryConnect(runConfig);
        m_usageCount[deviceHost] = 1;
    } else {
        ++m_usageCount[deviceHost];
    }

    return m_instances[deviceHost];
}

void BlackBerryConnect::cleanup(BlackBerryConnect *instance)
{
    const QString deviceHost = instance->m_deviceHost;
    QTC_ASSERT(m_usageCount.contains(deviceHost), return);

    --m_usageCount[deviceHost];
    QTC_ASSERT(m_usageCount[deviceHost] >= 0, return);

    if (m_usageCount[deviceHost] == 0) {
        m_instances.remove(deviceHost);
        m_usageCount.remove(deviceHost);
        instance->deleteLater();
    }
}

BlackBerryConnect::BlackBerryConnect(BlackBerryRunConfiguration *runConfig)
    : QObject()
    , m_connected(false)
{
    m_process = new QProcess(this);

    Utils::Environment env;
    Target *target = runConfig->target();
    if (target->activeBuildConfiguration())
        env = target->activeBuildConfiguration()->environment();

    m_process->setEnvironment(env.toStringList());
    m_connectCmd = env.searchInPath(QLatin1String(CONNECT_CMD));
    m_qnxHost = env.value(QLatin1String("QNX_HOST"));

    BlackBerryDeviceConfiguration::ConstPtr device
            = BlackBerryDeviceConfiguration::device(target->kit());
    m_deviceHost = device->sshParameters().host;
    m_password = device->sshParameters().password;
    m_publicKeyFile = device->sshParameters().privateKeyFile + QLatin1String(".pub");

    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(handleProcessFinished(int,QProcess::ExitStatus)));
}

void BlackBerryConnect::connectToDevice()
{
    if (m_connected) {
        emit connected();
        return;
    }

    QTC_ASSERT(!m_connectCmd.isEmpty() && !m_qnxHost.isEmpty(), return);

    // Since killing the blackberry-connect script won't kill the java process it launches, let's just call
    // the java process directly instead.
    QStringList connectArgs;
    connectArgs << QLatin1String("-Xmx512M");
    connectArgs << QLatin1String("-jar") << m_qnxHost  + QLatin1String("/usr/lib/Connect.jar");

    connectArgs << QLatin1String("-targetHost") << m_deviceHost;
    if (!m_password.isEmpty())
        connectArgs << QLatin1String("-password") << m_password;
    connectArgs << QLatin1String("-sshPublicKey") << m_publicKeyFile;

    m_process->start(m_connectCmd, connectArgs);
}

void BlackBerryConnect::disconnectFromDevice()
{
    if (m_process->state() != QProcess::Running)
        return;

    if (m_usageCount[m_deviceHost] == 1) {
        m_process->terminate();
        if (!m_process->waitForFinished(5000))
            m_process->kill();
    }
}

void BlackBerryConnect::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_connected = false;
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
        emit error(m_process->errorString());
}

void BlackBerryConnect::readStandardOutput()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(m_process->readLine());
        emit output(line, Utils::StdOutFormat);
        if (line.contains(QLatin1String(CONNECT_SUCCESS_MSG))) {
            m_connected = true;
            emit connected();
        }
    }
}

void BlackBerryConnect::readStandardError()
{
    m_process->setReadChannel(QProcess::StandardError);
    QStringList errorLines;

    while (m_process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(m_process->readLine());
        emit output(line, Utils::StdErrFormat);
        if (line.contains(QLatin1String("Error:")))
            errorLines << line.mid(7);
    }

    // TODO: Handle error messages better
    if (!errorLines.isEmpty())
        emit error(errorLines.join(QLatin1String("\n")));
}
