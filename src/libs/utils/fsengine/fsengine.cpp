/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "fsengine.h"

#ifdef QTC_UTILS_WITH_FSENGINE
#include "fsenginehandler.h"
#else
class Utils::Internal::FSEngineHandler
{};
#endif

#include <memory>

namespace Utils {

FSEngine::FSEngine()
    : m_engineHandler(std::make_unique<Internal::FSEngineHandler>())
{}

FSEngine::~FSEngine() {}

bool FSEngine::isAvailable()
{
#ifdef QTC_UTILS_WITH_FSENGINE
    return true;
#else
    return false;
#endif
}

FilePaths FSEngine::registeredDeviceRoots()
{
    return FSEngine::deviceRoots();
}

void FSEngine::addDevice(const FilePath &deviceRoot)
{
    deviceRoots().append(deviceRoot);
}

void FSEngine::removeDevice(const FilePath &deviceRoot)
{
    deviceRoots().removeAll(deviceRoot);
}

FilePaths &FSEngine::deviceRoots()
{
    static FilePaths g_deviceRoots;
    return g_deviceRoots;
}

QStringList &FSEngine::deviceSchemes()
{
    static QStringList g_deviceSchemes {"device"};
    return g_deviceSchemes;
}

void FSEngine::registerDeviceScheme(const QString &scheme)
{
    deviceSchemes().append(scheme);
}

void FSEngine::unregisterDeviceScheme(const QString &scheme)
{
    deviceSchemes().removeAll(scheme);
}

QStringList FSEngine::registeredDeviceSchemes()
{
    return FSEngine::deviceSchemes();
}

} // namespace Utils
