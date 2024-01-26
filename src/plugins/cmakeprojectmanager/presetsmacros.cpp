// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "presetsmacros.h"
#include "presetsparser.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/osspecificaspects.h>

using namespace Utils;

namespace CMakeProjectManager::Internal::CMakePresets::Macros {

static QString getHostSystemName(Utils::OsType osType)
{
    switch (osType) {
    case Utils::OsTypeWindows:
        return "Windows";
    case Utils::OsTypeLinux:
        return "Linux";
    case Utils::OsTypeMac:
        return "Darwin";
    case Utils::OsTypeOtherUnix:
        return "Unix";
    case Utils::OsTypeOther:
        return "Other";
    }
    return "Other";
}

static void expandAllButEnv(const PresetsDetails::ConfigurePreset &preset,
                            const Utils::FilePath &sourceDirectory,
                            QString &value)
{
    value.replace("${dollar}", "$");

    value.replace("${sourceDir}", sourceDirectory.toString());
    value.replace("${sourceParentDir}", sourceDirectory.parentDir().toString());
    value.replace("${sourceDirName}", sourceDirectory.fileName());

    value.replace("${presetName}", preset.name);
    value.replace("${fileDir}", preset.fileDir.path());
    if (preset.generator)
        value.replace("${generator}", preset.generator.value());

    value.replace("${hostSystemName}", getHostSystemName(sourceDirectory.osType()));
    value.replace("${pathListSep}",
                  Utils::OsSpecificAspects::pathListSeparator(sourceDirectory.osType()));
}

static void expandAllButEnv(const PresetsDetails::BuildPreset &preset,
                            const Utils::FilePath &sourceDirectory,
                            QString &value)
{
    value.replace("${dollar}", "$");

    value.replace("${sourceDir}", sourceDirectory.toString());
    value.replace("${fileDir}", preset.fileDir.path());
    value.replace("${sourceParentDir}", sourceDirectory.parentDir().toString());
    value.replace("${sourceDirName}", sourceDirectory.fileName());

    value.replace("${presetName}", preset.name);
    value.replace("${hostSystemName}", getHostSystemName(sourceDirectory.osType()));
    value.replace("${pathListSep}",
                  Utils::OsSpecificAspects::pathListSeparator(sourceDirectory.osType()));
}

static QString expandMacroEnv(const QString &macroPrefix,
                              const QString &value,
                              const std::function<QString(const QString &)> &op)
{
    const QString startToken = QString("$%1{").arg(macroPrefix);
    const QString endToken = QString("}");

    auto findMacro = [startToken,
                      endToken](const QString &str, qsizetype *pos, QString *ret) -> qsizetype {
        forever {
            qsizetype openPos = str.indexOf(startToken, *pos);
            if (openPos < 0)
                return 0;

            qsizetype varPos = openPos + startToken.length();
            qsizetype endPos = str.indexOf(endToken, varPos + 1);
            if (endPos < 0)
                return 0;

            *ret = str.mid(varPos, endPos - varPos);
            *pos = openPos;

            return endPos - openPos + endToken.length();
        }
    };

    QString result = value;
    QString macroName;

    bool done = true;
    do {
        done = true;
        for (qsizetype pos = 0; int len = findMacro(result, &pos, &macroName);) {
            result.replace(pos, len, op(macroName));
            pos += macroName.length();
            done = false;
        }
    } while (!done);

    return result;
}

static Environment getEnvCombined(const std::optional<Environment> &optPresetEnv,
                                  const Environment &env)
{
    Environment result = env;

    if (optPresetEnv) {
        optPresetEnv->forEachEntry([&result](const QString &key, const QString &value,
                                             bool enabled) {
            if (enabled)
                result.set(key, value);
        });
    }

    return result;
}

template<class PresetType>
void expand(const PresetType &preset, Environment &env, const FilePath &sourceDirectory)
{
    const Environment presetEnv = getEnvCombined(preset.environment, env);
    presetEnv.forEachEntry([&](const QString &key, const QString &value_, bool enabled) {
        if (!enabled)
            return;
        QString value = value_;
        expandAllButEnv(preset, sourceDirectory, value);
        value = expandMacroEnv("env", value, [presetEnv](const QString &macroName) {
            return presetEnv.value(macroName);
        });

        bool append = true;
        if (key.compare("PATH", Qt::CaseInsensitive) == 0) {
            const int index = value.indexOf("$penv{PATH}", 0, Qt::CaseInsensitive);
            if (index != 0)
                append = false;
            value.replace("$penv{PATH}", "", Qt::CaseInsensitive);
        }

        value = expandMacroEnv("penv", value, [env](const QString &macroName) {
            return env.value(macroName);
        });

        // Make sure to expand the CMake macros also for environment variables
        expandAllButEnv(preset, sourceDirectory, value);

        if (append)
            env.appendOrSet(key, value);
        else
            env.prependOrSet(key, value);
    });
}

template<class PresetType>
void expand(const PresetType &preset, EnvironmentItems &envItems, const FilePath &sourceDirectory)
{
    const Environment presetEnv = preset.environment ? *preset.environment : Environment();
    presetEnv.forEachEntry([&](const QString &key, const QString &value_, bool enabled) {
        if (!enabled)
            return;
        QString value = value_;
        expandAllButEnv(preset, sourceDirectory, value);
        value = expandMacroEnv("env", value, [presetEnv](const QString &macroName) {
            if (presetEnv.hasKey(macroName))
                return presetEnv.value(macroName);
            return QString("${%1}").arg(macroName);
        });

        auto operation = EnvironmentItem::Operation::SetEnabled;
        if (key.compare("PATH", Qt::CaseInsensitive) == 0) {
            operation = EnvironmentItem::Operation::Append;
            const int index = value.indexOf("$penv{PATH}", 0, Qt::CaseInsensitive);
            if (index != 0)
                operation = EnvironmentItem::Operation::Prepend;
            value.replace("$penv{PATH}", "", Qt::CaseInsensitive);
        }

        value = expandMacroEnv("penv", value, [](const QString &macroName) {
            return QString("${%1}").arg(macroName);
        });

        // Make sure to expand the CMake macros also for environment variables
        expandAllButEnv(preset, sourceDirectory, value);

        envItems.emplace_back(Utils::EnvironmentItem(key, value, operation));
    });
}

template<class PresetType>
void expand(const PresetType &preset,
            const Utils::Environment &env,
            const Utils::FilePath &sourceDirectory,
            QString &value)
{
    expandAllButEnv(preset, sourceDirectory, value);

    const Utils::Environment presetEnv = getEnvCombined(preset.environment, env);
    value = expandMacroEnv("env", value, [presetEnv](const QString &macroName) {
        return presetEnv.value(macroName);
    });

    value = expandMacroEnv("penv", value, [env](const QString &macroName) {
        return env.value(macroName);
    });

    // Make sure to expand the CMake macros also for environment variables
    expandAllButEnv(preset, sourceDirectory, value);
}

void updateToolchainFile(
    CMakeProjectManager::Internal::PresetsDetails::ConfigurePreset &configurePreset,
    const Utils::Environment &env,
    const Utils::FilePath &sourceDirectory,
    const Utils::FilePath &buildDirectory)
{
    if (!configurePreset.toolchainFile)
        return;

    QString toolchainFileName = configurePreset.toolchainFile.value();
    CMakePresets::Macros::expand(configurePreset, env, sourceDirectory, toolchainFileName);

    // Resolve the relative path first to source and afterwards to build directory
    Utils::FilePath toolchainFile = Utils::FilePath::fromString(toolchainFileName);
    if (toolchainFile.isRelativePath()) {
        for (const auto &path : {sourceDirectory, buildDirectory}) {
            Utils::FilePath probePath = path.resolvePath(toolchainFile);
            if (probePath.exists() && probePath != path) {
                toolchainFile = probePath;
                break;
            }
        }
    }

    if (!toolchainFile.exists())
        return;

    const QString toolchainFileString = toolchainFile.cleanPath().toString();

    // toolchainFile takes precedence to CMAKE_TOOLCHAIN_FILE
    CMakeConfig cache = configurePreset.cacheVariables ? configurePreset.cacheVariables.value()
                                                       : CMakeConfig();

    auto it = std::find_if(cache.begin(), cache.end(), [](const CMakeConfigItem &item) {
        return item.key == "CMAKE_TOOLCHAIN_FILE";
    });
    if (it != cache.end())
        it->value = toolchainFileString.toUtf8();
    else
        cache << CMakeConfigItem("CMAKE_TOOLCHAIN_FILE",
                                 CMakeConfigItem::FILEPATH,
                                 toolchainFileString.toUtf8());

    configurePreset.cacheVariables = cache;
}

void updateInstallDir(PresetsDetails::ConfigurePreset &configurePreset,
                      const Utils::Environment &env,
                      const Utils::FilePath &sourceDirectory)
{
    if (!configurePreset.installDir)
        return;

    QString installDirString = configurePreset.installDir.value();
    CMakePresets::Macros::expand(configurePreset, env, sourceDirectory, installDirString);

    // Resolve the relative path first to source and afterwards to build directory
    Utils::FilePath installDir = Utils::FilePath::fromString(installDirString);
    if (installDir.isRelativePath()) {
        Utils::FilePath probePath = sourceDirectory.resolvePath(installDir);
        if (probePath != sourceDirectory) {
            installDir = probePath;
        }
    }
    installDirString = installDir.cleanPath().toString();

    // installDir takes precedence to CMAKE_INSTALL_PREFIX
    CMakeConfig cache = configurePreset.cacheVariables ? configurePreset.cacheVariables.value()
                                                       : CMakeConfig();

    auto it = std::find_if(cache.begin(), cache.end(), [](const CMakeConfigItem &item) {
        return item.key == "CMAKE_INSTALL_PREFIX";
    });
    if (it != cache.end())
        it->value = installDirString.toUtf8();
    else
        cache << CMakeConfigItem("CMAKE_INSTALL_PREFIX",
                                 CMakeConfigItem::PATH,
                                 installDirString.toUtf8());

    configurePreset.cacheVariables = cache;
}


void updateCacheVariables(PresetsDetails::ConfigurePreset &configurePreset,
                          const Utils::Environment &env,
                          const Utils::FilePath &sourceDirectory)
{
    using namespace Utils;

    if (!configurePreset.cacheVariables)
        return;

    CMakeConfig cache = configurePreset.cacheVariables.value();

    static const QSet<QByteArray> pathKeys{"CMAKE_C_COMPILER",
                                           "CMAKE_CXX_COMPILER",
                                           "CMAKE_PREFIX_PATH",
                                           "CMAKE_FIND_ROOT_PATH",
                                           "CMAKE_MAKE_PROGRAM",
                                           "CMAKE_TOOLCHAIN_FILE",
                                           "QT_HOST_PATH",
                                           "QT_QMAKE_EXECUTABLE",
                                           "CMAKE_SYSROOT"};

    auto expandCacheValue =
        [configurePreset, env, sourceDirectory, cache](const QByteArray &key) {
        QString result = cache.stringValueOf(key);
        CMakePresets::Macros::expand(configurePreset, env, sourceDirectory, result);

        if (pathKeys.contains(key)) {
            const FilePaths paths = transform(result.split(";"), &FilePath::fromUserInput);
            result = transform(paths, &FilePath::path).join(";");
        }

        return result.toUtf8();
    };

    for (auto &item : cache)
        item.value = expandCacheValue(item.key);

    configurePreset.cacheVariables = cache;
}


template<class PresetType>
void expandConditionValues(const PresetType &preset,
                           const Utils::Environment &env,
                           const Utils::FilePath &sourceDirectory,
                           PresetsDetails::Condition &condition)
{
    if (condition.isEquals() || condition.isNotEquals()) {
        if (condition.lhs)
            expand(preset, env, sourceDirectory, condition.lhs.value());
        if (condition.rhs)
            expand(preset, env, sourceDirectory, condition.rhs.value());
    }

    if (condition.isInList() || condition.isNotInList()) {
        if (condition.string)
            expand(preset, env, sourceDirectory, condition.string.value());
        if (condition.list)
            for (QString &listValue : condition.list.value())
                expand(preset, env, sourceDirectory, listValue);
    }

    if (condition.isMatches() || condition.isNotMatches()) {
        if (condition.string)
            expand(preset, env, sourceDirectory, condition.string.value());
        if (condition.regex)
            expand(preset, env, sourceDirectory, condition.regex.value());
    }

    if (condition.isAnyOf() || condition.isAllOf()) {
        if (condition.conditions)
            for (PresetsDetails::Condition::ConditionPtr &c : condition.conditions.value())
                expandConditionValues(preset, env, sourceDirectory, *c);
    }

    if (condition.isNot()) {
        if (condition.condition)
            expandConditionValues(preset, env, sourceDirectory, *condition.condition.value());
    }
}

template<class PresetType>
bool evaluatePresetCondition(const PresetType &preset, const Utils::FilePath &sourceDirectory)
{
    if (!preset.condition)
        return true;

    Utils::Environment env = sourceDirectory.deviceEnvironment();
    expand(preset, env, sourceDirectory);

    PresetsDetails::Condition condition = preset.condition.value();
    expandConditionValues(preset, env, sourceDirectory, condition);

    return condition.evaluate();
}

// Expand for PresetsDetails::ConfigurePreset
template void expand<PresetsDetails::ConfigurePreset>(const PresetsDetails::ConfigurePreset &preset,
                                                      Utils::Environment &env,
                                                      const Utils::FilePath &sourceDirectory);

template void expand<PresetsDetails::ConfigurePreset>(const PresetsDetails::ConfigurePreset &preset,
                                                      Utils::EnvironmentItems &envItems,
                                                      const Utils::FilePath &sourceDirectory);

template void expand<PresetsDetails::ConfigurePreset>(const PresetsDetails::ConfigurePreset &preset,
                                                      const Utils::Environment &env,
                                                      const Utils::FilePath &sourceDirectory,
                                                      QString &value);

template bool evaluatePresetCondition<PresetsDetails::ConfigurePreset>(
    const PresetsDetails::ConfigurePreset &preset, const Utils::FilePath &sourceDirectory);

// Expand for PresetsDetails::BuildPreset
template void expand<PresetsDetails::BuildPreset>(const PresetsDetails::BuildPreset &preset,
                                                  Utils::Environment &env,
                                                  const Utils::FilePath &sourceDirectory);

template void expand<PresetsDetails::BuildPreset>(const PresetsDetails::BuildPreset &preset,
                                                  Utils::EnvironmentItems &envItems,
                                                  const Utils::FilePath &sourceDirectory);

template void expand<PresetsDetails::BuildPreset>(const PresetsDetails::BuildPreset &preset,
                                                  const Utils::Environment &env,
                                                  const Utils::FilePath &sourceDirectory,
                                                  QString &value);

template bool evaluatePresetCondition<PresetsDetails::BuildPreset>(
    const PresetsDetails::BuildPreset &preset, const Utils::FilePath &sourceDirectory);

} // namespace CMakeProjectManager::Internal::CMakePresets::Macros
