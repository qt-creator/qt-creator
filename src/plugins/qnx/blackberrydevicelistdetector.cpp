/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#include "blackberrydevicelistdetector.h"

#include "blackberryconfigurationmanager.h"
#include "blackberryndkprocess.h"

#include <utils/environment.h>

#include <QStringList>

namespace Qnx {
namespace Internal {

BlackBerryDeviceListDetector::BlackBerryDeviceListDetector(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, SIGNAL(readyRead()), this, SLOT(processReadyRead()));
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished()));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processFinished()));
}

void BlackBerryDeviceListDetector::detectDeviceList()
{
    if (m_process->state() != QProcess::NotRunning)
        return;

    m_process->setEnvironment(Utils::EnvironmentItem::toStringList(
             BlackBerryConfigurationManager::instance()->defaultConfigurationEnv()));
    const QString command = BlackBerryNdkProcess::resolveNdkToolPath(QLatin1String("blackberry-deploy"));
    QStringList arguments;
    arguments << QLatin1String("-devices");

    m_process->start(command, arguments, QIODevice::ReadWrite | QIODevice::Unbuffered);
}

void BlackBerryDeviceListDetector::processReadyRead()
{
    while (m_process->canReadLine())
        processData(readProcessLine());
}

void BlackBerryDeviceListDetector::processFinished()
{
    while (!m_process->atEnd())
        processData(readProcessLine());
    emit finished();
}

const QString BlackBerryDeviceListDetector::readProcessLine()
{
    // we assume that the process output is ASCII only
    QByteArray bytes = m_process->readLine();
    while (bytes.endsWith('\r')  ||  bytes.endsWith('\n'))
        bytes.chop(1);
    return QString::fromLocal8Bit(bytes);
}

void BlackBerryDeviceListDetector::processData(const QString &line)
{
    // line format is: deviceName,deviceHostNameOrIP,deviceType,versionIfSimulator
    QStringList list = line.split(QLatin1String(","));
    if (list.count() == 4)
        emit deviceDetected (list[0], list[1], QLatin1String("Simulator") == list[2]);
}

} // namespace Internal
} // namespace Qnx
