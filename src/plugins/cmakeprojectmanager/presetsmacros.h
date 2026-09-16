// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/environmentfwd.h>

#include <QByteArray>

namespace Utils {
class Environment;
class FilePath;
}

namespace CMakeProjectManager::Internal {

namespace PresetsDetails {
class ConfigurePreset;
class TestPreset;
}

namespace CMakePresets::Macros {
/**
 * Returns the environment without the variables that a "null" value shadowed. Such a value means
 * that the preset does not set the variable, so applying the environment to another one must not
 * remove it from there.
 */
Utils::Environment withoutUnsetVariables(const Utils::Environment &environment);

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
 * Expands the CMakePresets macros that do not belong to a preset, for the file names of the
 * "include" section. $penv{PATH} is taken from the environment of the source directory.
 */
void expandFileMacros(const Utils::FilePath &sourceDirectory,
                      const Utils::FilePath &fileDir,
                      QString &value);

/**
 * Whether updateToolchainFile() or updateInstallDir() already expanded the macros of the @a key
 * cache variable. Expanding it a second time would also expand the dollar sign that a ${dollar}
 * produced.
 */
bool isExpandedCacheVariable(const PresetsDetails::ConfigurePreset &configurePreset,
                             const QByteArray &key);

/**
 * Updates the cacheVariables parameter of the configurePreset with the expandned toolchainFile parameter.
 * Including macro expansion and relative paths resolving.
 */
void updateToolchainFile(PresetsDetails::ConfigurePreset &configurePreset,
                         const Utils::Environment &env,
                         const Utils::FilePath &sourceDirectory,
                         const Utils::FilePath &buildDirectory);

/**
 * Updates the cacheVariables parameter of the configurePreset with the expanded installDir parameter.
 * Including macro expansion and relative paths resolving.
 */
void updateInstallDir(PresetsDetails::ConfigurePreset &configurePreset,
                      const Utils::Environment &env,
                      const Utils::FilePath &sourceDirectory);

/**
 * Updates the cacheVariables parameter of the configurePreset with the expanded prameter values.
 * Including macro expansion and relative paths resolving.
 */
void updateCacheVariables(PresetsDetails::ConfigurePreset &configurePreset,
                          const Utils::Environment &env,
                          const Utils::FilePath &sourceDirectory);

/**
 * Expands the macros of the fields of the testPreset that end up on the ctest command line or in
 * the environment of the test process.
 */
void expandTestPreset(PresetsDetails::TestPreset &testPreset,
                      const Utils::Environment &env,
                      const Utils::FilePath &sourceDirectory);

/**
 * Expands the condition values and then evaluates the condition object of the preset and returns
 * the boolean result.
 */
template<class PresetType>
bool evaluatePresetCondition(const PresetType &preset, const Utils::FilePath &sourceDirectory);

} // namespace CMakePresets::Macros

} // namespace CMakeProjectManager::Internal
