// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeconfigitem.h"

#include <utils/environment.h>
#include <utils/filepath.h>

#include <QHash>
#include <QVersionNumber>

namespace CMakeProjectManager::Internal {

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

class Condition {
public:
    QString type;

    bool isNull() const { return type == "null"; }
    bool isConst() const  { return type == "const"; }
    bool isEquals() const  { return type == "equals"; }
    bool isNotEquals() const  { return type == "notEquals"; }
    bool isInList() const  { return type == "inList"; }
    bool isNotInList() const { return type == "notInList"; }
    bool isMatches() const { return type == "matches"; }
    bool isNotMatches() const { return type == "notMatches"; }
    bool isAnyOf() const { return type == "anyOf"; }
    bool isAllOf() const { return type == "allOf"; }
    bool isNot() const { return type == "not"; }

    bool evaluate() const;

    // const
    std::optional<bool> constValue;

    // equals, notEquals
    std::optional<QString> lhs;
    std::optional<QString> rhs;

    // inList, notInList
    std::optional<QString> string;
    std::optional<QStringList> list;

    // matches, notMatches
    std::optional<QString> regex;

    using ConditionPtr = std::shared_ptr<Condition>;

    // anyOf, allOf
    std::optional<std::vector<ConditionPtr>> conditions;

    // not
    std::optional<ConditionPtr> condition;
};

class ConfigurePreset {
public:
    void inheritFrom(const ConfigurePreset &other);

    QString name;
    std::optional<bool> hidden = false;
    std::optional<QStringList> inherits;
    std::optional<Condition> condition;
    std::optional<QHash<QString, QString>> vendor;
    std::optional<QString> displayName;
    std::optional<QString> description;
    std::optional<QString> generator;
    std::optional<ValueStrategyPair> architecture;
    std::optional<ValueStrategyPair> toolset;
    std::optional<QString> toolchainFile;
    std::optional<QString> binaryDir;
    std::optional<QString> installDir;
    std::optional<QString> cmakeExecutable;
    std::optional<CMakeConfig> cacheVariables;
    std::optional<Utils::Environment> environment;
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
    std::optional<Condition> condition;
    std::optional<QHash<QString, QString>> vendor;
    std::optional<QString> displayName;
    std::optional<QString> description;
    std::optional<Utils::Environment> environment;
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
    QList<PresetsDetails::ConfigurePreset> configurePresets;
    QList<PresetsDetails::BuildPreset> buildPresets;
};

class PresetsParser
{
    PresetsData m_presetsData;
public:
    bool parse(Utils::FilePath const &jsonFile, QString &errorMessage, int &errorLine);

    const PresetsData &presetsData() const;
};

} // CMakeProjectManager::Internal
