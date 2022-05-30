// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/environmentfwd.h>

namespace Utils {
class Environment;
class FilePath;
}

namespace CMakeProjectManager::Internal {

namespace PresetsDetails {
class ConfigurePreset;
}

namespace CMakePresets::Macros {
/**
 * Expands the CMakePresets Macros using Utils::Environment as target and source for parent environment values.
 * $penv{PATH} is taken from Utils::Environment
 */
void expand(const PresetsDetails::ConfigurePreset &configurePreset,
            Utils::Environment &env,
            const Utils::FilePath &sourceDirectory);

/**
 * Expands the CMakePresets Macros using Utils::Environment as target
 * $penv{PATH} is replaced with Qt Creator macros ${PATH}
 */
void expand(const PresetsDetails::ConfigurePreset &configurePreset,
            Utils::EnvironmentItems &envItems,
            const Utils::FilePath &sourceDirectory);

/**
 * Expands the CMakePresets macros inside the @value QString parameter.
 */
void expand(const PresetsDetails::ConfigurePreset &configurePreset,
            const Utils::Environment &env,
            const Utils::FilePath &sourceDirectory,
            QString &value);

} // namespace CMakePresets::Macros

} // namespace CMakeProjectManager::Internal
