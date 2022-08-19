// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

#include <QCache>
#include <QVersionNumber>

namespace Utils {
class Environment;
}

namespace WebAssembly {
namespace Internal {

class WebAssemblyEmSdk
{
public:
    static bool isValid(const Utils::FilePath &sdkRoot);
    static void addToEnvironment(const Utils::FilePath &sdkRoot, Utils::Environment &env);
    static QVersionNumber version(const Utils::FilePath &sdkRoot);
    static void registerEmSdk(const Utils::FilePath &sdkRoot);
    static Utils::FilePath registeredEmSdk();
    static void clearCaches();
};

} // namespace Internal
} // namespace WebAssembly
