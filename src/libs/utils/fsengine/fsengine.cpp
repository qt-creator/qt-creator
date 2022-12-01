// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
    if (deviceRoot.exists())
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

void FSEngine::registerDeviceScheme(const QStringView scheme)
{
    deviceSchemes().append(scheme.toString());
}

void FSEngine::unregisterDeviceScheme(const QStringView scheme)
{
    deviceSchemes().removeAll(scheme.toString());
}

QStringList FSEngine::registeredDeviceSchemes()
{
    return FSEngine::deviceSchemes();
}

} // namespace Utils
