// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"
#include "../utils_global.h"

#include <memory>

namespace Utils {

namespace Internal {
class FSEngineHandler;
}

class QTCREATOR_UTILS_EXPORT FSEngine
{
    friend class Internal::FSEngineHandler;

public:
    FSEngine();
    ~FSEngine();

public:
    static bool isAvailable();

    static Utils::FilePaths registeredDeviceRoots();
    static void addDevice(const Utils::FilePath &deviceRoot);
    static void removeDevice(const Utils::FilePath &deviceRoot);

    static void registerDeviceScheme(const QStringView scheme);
    static void unregisterDeviceScheme(const QStringView scheme);
    static QStringList registeredDeviceSchemes();

private:
    std::unique_ptr<Internal::FSEngineHandler> m_engineHandler;
};

} // namespace Utils
