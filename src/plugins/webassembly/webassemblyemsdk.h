// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVersionNumber>

namespace Utils {
class Environment;
class FilePath;
}

namespace WebAssembly::Internal::WebAssemblyEmSdk {

bool isValid(const Utils::FilePath &sdkRoot);
void parseEmSdkEnvOutputAndAddToEnv(const QString &output, Utils::Environment &env);
void addToEnvironment(const Utils::FilePath &sdkRoot, Utils::Environment &env);
QVersionNumber version(const Utils::FilePath &sdkRoot);
void clearCaches();

} // WebAssembly::Internal::WebAssemblyEmSdk
