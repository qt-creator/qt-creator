// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "devcontainer_global.h"
#include "devcontainerconfig.h"

#include <utils/result.h>

#include <QJsonObject>

namespace DevContainer {

struct DEVCONTAINER_EXPORT FeatureOption
{
    static FeatureOption fromJson(
        const QJsonObject &obj, const JsonStringToString &jsonStringToString);

    QString type;
    QVariant defaultValue;
    QString description;
    QStringList enumValues;
    QStringList proposals;
};

struct DEVCONTAINER_EXPORT Feature
{
    static Utils::Result<Feature> fromJson(
        const QJsonObject &obj, const JsonStringToString &jsonStringToString);

    static Utils::Result<Feature> fromJson(
        const QByteArray &data, const JsonStringToString &jsonStringToString);

    // Identification fields
    QString id;
    QStringList legacyIds;
    QString version;

    // Meta data fields
    QString description;
    QString documentationURL;
    QString licenseURL;
    QString name;
    QStringList keywords;
    bool deprecated = false;

    // Fields that are added / merged onto the container config
    QString entrypoint;
    QStringList capAdd;
    QStringList securityOpt;
    std::map<QString, QString> containerEnv;
    QJsonObject customizations;
    std::vector<std::variant<Mount, QString>> mounts;

    bool init = false;
    bool privileged = false;

    std::optional<Command> onCreateCommand;
    std::optional<Command> updateContentCommand;
    std::optional<Command> postCreateCommand;
    std::optional<Command> postStartCommand;
    std::optional<Command> postAttachCommand;

    // Installation order fields (https://containers.dev/implementors/features/#installation-order)
    QStringList installsAfter;
    QList<FeatureDependency> dependsOn;

    // Options for the feature (https://containers.dev/implementors/features/#options-property)
    QMap<QString, FeatureOption> options;
};

DEVCONTAINER_EXPORT QJsonObject mergeCustomizations(QJsonObject left, const QJsonObject &right);

} // namespace DevContainer
