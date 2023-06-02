// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fsengine.h"

#ifdef QTC_UTILS_WITH_FSENGINE
#include "fsenginehandler.h"
#else
class Utils::Internal::FSEngineHandler
{};
#endif

#include <QMutex>

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

template<class T>
class Locked
{
public:
    Locked(QMutex *mutex, T &object)
        : m_object(object)
        , m_locker(mutex)
    {}

    T *operator->() const noexcept { return &m_object; }
    const T operator*() const noexcept { return m_object; }

private:
    T &m_object;
    QMutexLocker<QMutex> m_locker;
};

static Locked<Utils::FilePaths> deviceRoots()
{
    static FilePaths g_deviceRoots;
    static QMutex mutex;
    return {&mutex, g_deviceRoots};
}

static Locked<QStringList> deviceSchemes()
{
    static QStringList g_deviceSchemes{"device"};
    static QMutex mutex;
    return {&mutex, g_deviceSchemes};
}

FilePaths FSEngine::registeredDeviceRoots()
{
    return *deviceRoots();
}

void FSEngine::addDevice(const FilePath &deviceRoot)
{
    deviceRoots()->append(deviceRoot);
}

void FSEngine::removeDevice(const FilePath &deviceRoot)
{
    deviceRoots()->removeAll(deviceRoot);
}

void FSEngine::registerDeviceScheme(const QStringView scheme)
{
    deviceSchemes()->append(scheme.toString());
}

void FSEngine::unregisterDeviceScheme(const QStringView scheme)
{
    deviceSchemes()->removeAll(scheme.toString());
}

QStringList FSEngine::registeredDeviceSchemes()
{
    return *deviceSchemes();
}

} // namespace Utils
