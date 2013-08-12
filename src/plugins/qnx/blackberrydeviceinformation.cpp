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

void BlackBerryDeviceInformation::resetResults()
{
    m_devicePin.clear();
    m_deviceOS.clear();
    m_hardwareId.clear();
    m_debugTokenAuthor.clear();
    m_scmBundle.clear();
    m_hostName.clear();
    m_debugTokenValid = false;
    m_isSimulator = false;
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

QString BlackBerryDeviceInformation::debugTokenAuthor() const
{
    return m_debugTokenAuthor;
}

QString BlackBerryDeviceInformation::scmBundle() const
{
    return m_scmBundle;
}

QString BlackBerryDeviceInformation::hostName() const
{
    return m_hostName;
}

bool BlackBerryDeviceInformation::debugTokenValid() const
{
    return m_debugTokenValid;
}

bool BlackBerryDeviceInformation::isSimulator() const
{
    return m_isSimulator;
}

void BlackBerryDeviceInformation::processData(const QString &line)
{
    if (line.startsWith(QLatin1String("devicepin::0x")))
        m_devicePin = line.mid(QLatin1String("devicepin::0x").size()).trimmed();
    else if (line.startsWith(QLatin1String("device_os::")))
        m_deviceOS = line.mid(QLatin1String("device_os::").size()).trimmed();
    else if (line.startsWith(QLatin1String("hardwareid::")))
        m_hardwareId = line.mid(QLatin1String("hardwareid::").size()).trimmed();
    else if (line.startsWith(QLatin1String("debug_token_author::")))
        m_debugTokenAuthor = line.mid(QLatin1String("debug_token_author::").size()).trimmed();
    else if (line.startsWith(QLatin1String("debug_token_valid:b:")))
        m_debugTokenValid = line.mid(QLatin1String("debug_token_valid:b:").size()).trimmed()
                == QLatin1String("true");
    else if (line.startsWith(QLatin1String("simulator:b:")))
        m_isSimulator = line.mid(QLatin1String("simulator:b:").size()).trimmed()
                == QLatin1String("true");
    else if (line.startsWith(QLatin1String("scmbundle::")))
        m_scmBundle = line.mid(QLatin1String("scmbundle::").size()).trimmed();
    else if (line.startsWith(QLatin1String("hostname::")))
        m_hostName = line.mid(QLatin1String("hostname::").size()).trimmed();
}

} // namespace Internal
} // namespace Qnx
