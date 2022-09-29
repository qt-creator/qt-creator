// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include "cmakeconfigitem.h"

#include <QHash>
#include <QString>
#include <QVersionNumber>

namespace CMakeProjectManager {
namespace Internal {

namespace PresetsDetails {

class ValueStrategyPair {
public:
    std::optional<QString> value;
    enum class Strategy : bool { set, external };
    std::optional<Strategy> strategy;
};

class Warnings {
public:
    std::optional<bool> dev;
    std::optional<bool> deprecated;
    std::optional<bool> uninitialized = false;
    std::optional<bool> unusedCli = true;
    std::optional<bool> systemVars = false;
};

class Errors {
public:
    std::optional<bool> dev;
    std::optional<bool> deprecated;
};

class Debug {
public:
    std::optional<bool> output = false;
    std::optional<bool> tryCompile = false;
    std::optional<bool> find = false;
};

class ConfigurePreset {
public:
    void inheritFrom(const ConfigurePreset &other);

    QString name;
    std::optional<bool> hidden = false;
    std::optional<QStringList> inherits;
    std::optional<QHash<QString, QString>> vendor;
    std::optional<QString> displayName;
    std::optional<QString> description;
    std::optional<QString> generator;
    std::optional<ValueStrategyPair> architecture;
    std::optional<ValueStrategyPair> toolset;
    std::optional<QString> binaryDir;
    std::optional<QString> cmakeExecutable;
    std::optional<CMakeConfig> cacheVariables;
    std::optional<QHash<QString, QString>> environment;
    std::optional<Warnings> warnings;
    std::optional<Errors> errors;
    std::optional<Debug> debug;
};

class BuildPreset {
public:
    void inheritFrom(const BuildPreset &other);

    QString name;
    std::optional<bool> hidden = false;
    std::optional<QStringList> inherits;
    std::optional<QHash<QString, QString>> vendor;
    std::optional<QString> displayName;
    std::optional<QString> description;
    std::optional<QHash<QString, QString>> environment;
    std::optional<QString> configurePreset;
    std::optional<bool> inheritConfigureEnvironment = true;
    std::optional<int> jobs;
    std::optional<QStringList> targets;
    std::optional<QString> configuration;
    std::optional<bool> verbose;
    std::optional<bool> cleanFirst;
    std::optional<QStringList> nativeToolOptions;
};

} // namespace PresetsDetails

class PresetsData
{
public:
    int version = 0;
    QVersionNumber cmakeMinimimRequired;
    QHash<QString, QString> vendor;
    std::vector<PresetsDetails::ConfigurePreset> configurePresets;
    std::vector<PresetsDetails::BuildPreset> buildPresets;
};

class PresetsParser
{
    PresetsData m_presetsData;
public:
    bool parse(Utils::FilePath const &jsonFile, QString &errorMessage, int &errorLine);

    const PresetsData &presetsData() const;
};

} // namespace Internal
} // namespace CMakeProjectManager
