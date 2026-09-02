// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

#include <utils/filepath.h>

namespace HarmonyOs::Internal {

// The application bundle name from the generated harmonyos-build/AppScope/app.json5.
QString bundleName(const Utils::FilePath &buildDir);

// The library the build produced, as harmonydeployqt was told about it. Empty when the
// deployment settings are not there yet.
Utils::FilePath applicationLibrary(const Utils::FilePath &buildDir);

void setupHarmonyOsRunSupport();

} // namespace HarmonyOs::Internal
