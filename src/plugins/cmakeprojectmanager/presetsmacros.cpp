// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "presetsmacros.h"
#include "presetsparser.h"

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QRegularExpression>

namespace CMakeProjectManager::Internal::CMakePresets::Macros {

QString getHostSystemName()
{
    Utils::OsType osType = Utils::HostOsInfo::hostOs();

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

void expandAllButEnv(const PresetsDetails::ConfigurePreset &preset,
                     const Utils::FilePath &sourceDirectory,
                     QString &value)
{
    value.replace("${dollar}", "$");

    value.replace("${sourceDir}", sourceDirectory.toString());
    value.replace("${sourceParentDir}", sourceDirectory.parentDir().toString());
    value.replace("${sourceDirName}", sourceDirectory.fileName());

    value.replace("${presetName}", preset.name);
    if (preset.generator)
        value.replace("${generator}", preset.generator.value());

    value.replace("${hostSystemName}", getHostSystemName());
}

void expandAllButEnv(const PresetsDetails::BuildPreset &preset,
                     const Utils::FilePath &sourceDirectory,
                     QString &value)
{
    value.replace("${dollar}", "$");

    value.replace("${sourceDir}", sourceDirectory.toString());
    value.replace("${sourceParentDir}", sourceDirectory.parentDir().toString());
    value.replace("${sourceDirName}", sourceDirectory.fileName());

    value.replace("${presetName}", preset.name);
}

template<class PresetType>
void expand(const PresetType &preset,
            Utils::Environment &env,
            const Utils::FilePath &sourceDirectory)
{
    const QHash<QString, QString> presetEnv = preset.environment ? preset.environment.value()
                                                                 : QHash<QString, QString>();
    for (auto it = presetEnv.constKeyValueBegin(); it != presetEnv.constKeyValueEnd(); ++it) {
        const QString key = it->first;
        QString value = it->second;

        expandAllButEnv(preset, sourceDirectory, value);

        QRegularExpression envRegex(R"((\$env\{(\w+)\}))");
        for (const QRegularExpressionMatch &match : envRegex.globalMatch(value)) {
            if (match.captured(2) != key)
                value.replace(match.captured(1), presetEnv.value(match.captured(2)));
            else
                value.replace(match.captured(1), "");
        }

        QString sep;
        bool append = true;
        if (key.compare("PATH", Qt::CaseInsensitive) == 0) {
            sep = Utils::OsSpecificAspects::pathListSeparator(env.osType());
            const int index = value.indexOf("$penv{PATH}", 0, Qt::CaseInsensitive);
            if (index != 0)
                append = false;
            value.replace("$penv{PATH}", "", Qt::CaseInsensitive);
        }

        QRegularExpression penvRegex(R"((\$penv\{(\w+)\}))");
        for (const QRegularExpressionMatch &match : penvRegex.globalMatch(value))
            value.replace(match.captured(1), env.value(match.captured(2)));

        if (append)
            env.appendOrSet(key, value, sep);
        else
            env.prependOrSet(key, value, sep);
    }
}

template<class PresetType>
void expand(const PresetType &preset,
            Utils::EnvironmentItems &envItems,
            const Utils::FilePath &sourceDirectory)
{
    const QHash<QString, QString> presetEnv = preset.environment ? preset.environment.value()
                                                                 : QHash<QString, QString>();

    for (auto it = presetEnv.constKeyValueBegin(); it != presetEnv.constKeyValueEnd(); ++it) {
        const QString key = it->first;
        QString value = it->second;

        expandAllButEnv(preset, sourceDirectory, value);

        QRegularExpression envRegex(R"((\$env\{(\w+)\}))");
        for (const QRegularExpressionMatch &match : envRegex.globalMatch(value)) {
            if (match.captured(2) != key)
                value.replace(match.captured(1), presetEnv.value(match.captured(2)));
            else
                value.replace(match.captured(1), "");
        }

        auto operation = Utils::EnvironmentItem::Operation::SetEnabled;
        if (key.compare("PATH", Qt::CaseInsensitive) == 0) {
            operation = Utils::EnvironmentItem::Operation::Append;
            const int index = value.indexOf("$penv{PATH}", 0, Qt::CaseInsensitive);
            if (index != 0)
                operation = Utils::EnvironmentItem::Operation::Prepend;
            value.replace("$penv{PATH}", "", Qt::CaseInsensitive);
        }

        QRegularExpression penvRegex(R"((\$penv\{(\w+)\}))");
        for (const QRegularExpressionMatch &match : penvRegex.globalMatch(value))
            value.replace(match.captured(1), QString("${%1}").arg(match.captured(2)));

        envItems.emplace_back(Utils::EnvironmentItem(key, value, operation));
    }
}

template<class PresetType>
void expand(const PresetType &preset,
            const Utils::Environment &env,
            const Utils::FilePath &sourceDirectory,
            QString &value)
{
    expandAllButEnv(preset, sourceDirectory, value);

    const QHash<QString, QString> presetEnv = preset.environment ? preset.environment.value()
                                                                 : QHash<QString, QString>();

    QRegularExpression envRegex(R"((\$env\{(\w+)\}))");
    for (const QRegularExpressionMatch &match : envRegex.globalMatch(value))
        value.replace(match.captured(1), presetEnv.value(match.captured(2)));

    QRegularExpression penvRegex(R"((\$penv\{(\w+)\}))");
    for (const QRegularExpressionMatch &match : penvRegex.globalMatch(value))
        value.replace(match.captured(1), env.value(match.captured(2)));
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
            Utils::FilePath probePath = toolchainFile.resolvePath(path);
            if (probePath.exists() && probePath != path) {
                toolchainFile = probePath;
                break;
            }
        }
    }

    if (!toolchainFile.exists())
        return;

    // toolchainFile takes precedence to CMAKE_TOOLCHAIN_FILE
    CMakeConfig cache = configurePreset.cacheVariables ? configurePreset.cacheVariables.value()
                                                       : CMakeConfig();

    auto it = std::find_if(cache.begin(), cache.end(), [](const CMakeConfigItem &item) {
        return item.key == "CMAKE_TOOLCHAIN_FILE";
    });
    if (it != cache.end())
        it->value = toolchainFile.toString().toUtf8();
    else
        cache << CMakeConfigItem("CMAKE_TOOLCHAIN_FILE",
                                 CMakeConfigItem::FILEPATH,
                                 toolchainFile.toString().toUtf8());

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

    Utils::Environment env = Utils::Environment::systemEnvironment();
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
