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
template<class PresetType>
void expand(const PresetType &preset,
            Utils::Environment &env,
            const Utils::FilePath &sourceDirectory);

/**
 * Expands the CMakePresets Macros using Utils::Environment as target
 * $penv{PATH} is replaced with Qt Creator macros ${PATH}
 */
template<class PresetType>
void expand(const PresetType &preset,
            Utils::EnvironmentItems &envItems,
            const Utils::FilePath &sourceDirectory);

/**
 * Expands the CMakePresets macros inside the @value QString parameter.
 */
template<class PresetType>
void expand(const PresetType &preset,
            const Utils::Environment &env,
            const Utils::FilePath &sourceDirectory,
            QString &value);

/**
 * Updates the cacheVariables parameter of the configurePreset with the expandned toolchainFile parameter.
 * Including macro expansion and relative paths resolving.
 */
void updateToolchainFile(PresetsDetails::ConfigurePreset &configurePreset,
                         const Utils::Environment &env,
                         const Utils::FilePath &sourceDirectory,
                         const Utils::FilePath &buildDirectory);

/**
 * Expands the condition values and then evaluates the condition object of the preset and returns
 * the boolean result.
 */
template<class PresetType>
bool evaluatePresetCondition(const PresetType &preset, const Utils::FilePath &sourceDirectory);

} // namespace CMakePresets::Macros

} // namespace CMakeProjectManager::Internal
