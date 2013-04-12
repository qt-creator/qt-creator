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

#include "blackberrydeviceinformation.h"

namespace {
static const char PROCESS_NAME[] = "blackberry-deploy";
static const char ERR_NO_ROUTE_HOST[] = "Cannot connect";
static const char ERR_AUTH_FAILED[] = "Authentication failed";
static const char ERR_DEVELOPMENT_MODE_DISABLED[] = "Device is not in the Development Mode";
}

namespace Qnx {
namespace Internal {

BlackBerryDeviceInformation::BlackBerryDeviceInformation(QObject *parent) :
    BlackBerryNdkProcess(QLatin1String(PROCESS_NAME), parent)
{
    addErrorStringMapping(QLatin1String(ERR_NO_ROUTE_HOST), NoRouteToHost);
    addErrorStringMapping(QLatin1String(ERR_AUTH_FAILED), AuthenticationFailed);
    addErrorStringMapping(QLatin1String(ERR_DEVELOPMENT_MODE_DISABLED), DevelopmentModeDisabled);
}

void BlackBerryDeviceInformation::setDeviceTarget(const QString &deviceIp, const QString &devicePassword)
{
    QStringList arguments;

    arguments << QLatin1String("-listDeviceInfo")
              << QLatin1String("-device")
              << deviceIp
              << QLatin1String("-password")
              << devicePassword;

    start(arguments);
}

QString BlackBerryDeviceInformation::devicePin() const
{
    return m_devicePin;
}

QString BlackBerryDeviceInformation::deviceOS() const
{
    return m_deviceOS;
}

QString BlackBerryDeviceInformation::hardwareId() const
{
    return m_hardwareId;
}

void BlackBerryDeviceInformation::processData(const QString &line)
{
    if (line.startsWith(QLatin1String("devicepin::")))
        m_devicePin = line.split(QLatin1String("::0x")).at(1).trimmed();
    else if (line.startsWith(QLatin1String("device_os::")))
        m_deviceOS = line.split(QLatin1String("::")).at(1).trimmed();
    else if (line.startsWith(QLatin1String("hardwareid::")))
        m_hardwareId = line.split(QLatin1String("::")).at(1).trimmed();
}

} // namespace Internal
} // namespace Qnx
