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
    BlackBerryNdkProcess(QLatin1String(PROCESS_NAME), parent),
    m_debugTokenValid(false), m_isSimulator(false), m_isProductionDevice(true)
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
    m_debugTokenValidationError.clear();
    m_scmBundle.clear();
    m_hostName.clear();
    m_debugTokenValid = false;
    m_isSimulator = false;
    m_isProductionDevice = true;
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

QString BlackBerryDeviceInformation::debugTokenValidationError() const
{
    return m_debugTokenValidationError;
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

bool BlackBerryDeviceInformation::isProductionDevice() const
{
    return m_isProductionDevice;
}

void BlackBerryDeviceInformation::processData(const QString &line)
{
    static const QString devicepin = QLatin1String("devicepin::0x");
    static const QString device_os = QLatin1String("device_os::");
    static const QString hardwareid = QLatin1String("hardwareid::");
    static const QString debug_token_author = QLatin1String("[n]debug_token_author::");
    static const QString debug_token_validation_error = QLatin1String("[n]debug_token_validation_error::");
    static const QString debug_token_valid = QLatin1String("[n]debug_token_valid:b:");
    static const QString simulator = QLatin1String("simulator:b:");
    static const QString scmbundle = QLatin1String("scmbundle::");
    static const QString hostname = QLatin1String("hostname::");
    static const QString production_device = QLatin1String("production_device:b:");

    if (line.startsWith(devicepin))
        m_devicePin = line.mid(devicepin.size()).trimmed();
    else if (line.startsWith(device_os))
        m_deviceOS = line.mid(device_os.size()).trimmed();
    else if (line.startsWith(hardwareid))
        m_hardwareId = line.mid(hardwareid.size()).trimmed();
    else if (line.startsWith(debug_token_author))
        m_debugTokenAuthor = line.mid(debug_token_author.size()).trimmed();
    else if (line.startsWith(debug_token_validation_error))
        m_debugTokenValidationError = line.mid(debug_token_validation_error.size()).trimmed();
    else if (line.startsWith(debug_token_valid))
        m_debugTokenValid = line.mid(debug_token_valid.size()).trimmed() == QLatin1String("true");
    else if (line.startsWith(simulator))
        m_isSimulator = line.mid(simulator.size()).trimmed() == QLatin1String("true");
    else if (line.startsWith(scmbundle))
        m_scmBundle = line.mid(scmbundle.size()).trimmed();
    else if (line.startsWith(hostname))
        m_hostName = line.mid(hostname.size()).trimmed();
    else if (line.startsWith(production_device))
        m_isProductionDevice = line.mid(production_device.size()).trimmed() == QLatin1String("true");
}

} // namespace Internal
} // namespace Qnx
