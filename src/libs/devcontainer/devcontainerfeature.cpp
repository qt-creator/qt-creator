// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerfeature.h"

#include "devcontainertr.h"

#include <utils/stringutils.h>

using namespace Utils;

namespace DevContainer {

Result<Feature> Feature::fromJson(
    const QByteArray &data, const JsonStringToString &jsonStringToString)
{
    const QByteArray cleanedInput = cleanJson(data);

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(cleanedInput, &error);
    if (error.error != QJsonParseError::NoError) {
        return ResultError(
            Tr::tr("Failed to parse the development container feature file: %1")
                .arg(error.errorString()));
    }

    if (!doc.isObject())
        return ResultError(Tr::tr("Invalid development container JSON file: expected an object."));

    QJsonObject json = doc.object();
    return Feature::fromJson(json, jsonStringToString);
}

Result<Feature> Feature::fromJson(
    const QJsonObject &obj, const JsonStringToString &jsonStringToString)
{
    Feature f;
    f.id = jsonStringToString(obj.value("id"));
    f.version = jsonStringToString(obj.value("version"));

    f.description = jsonStringToString(obj.value("description"));
    f.documentationURL = jsonStringToString(obj.value("documentationURL"));
    f.entrypoint = jsonStringToString(obj.value("entrypoint"));
    f.licenseURL = jsonStringToString(obj.value("licenseURL"));
    f.name = jsonStringToString(obj.value("name"));

    for (const auto &val : obj.value("capAdd").toArray())
        f.capAdd.append(jsonStringToString(val));

    for (const auto &val : obj.value("keywords").toArray())
        f.keywords.append(jsonStringToString(val));

    for (const auto &val : obj.value("installsAfter").toArray())
        f.installsAfter.append(jsonStringToString(val));

    for (const auto &val : obj.value("securityOpt").toArray())
        f.securityOpt.append(jsonStringToString(val));

    for (const auto &val : obj.value("legacyIds").toArray())
        f.legacyIds.append(jsonStringToString(val));

    auto containerEnv = obj.value("containerEnv").toObject();
    for (auto it = containerEnv.begin(); it != containerEnv.end(); ++it)
        f.containerEnv[it.key()] = jsonStringToString(it.value());

    f.customizations = obj.value("customizations").toObject();

    if (obj.contains("dependsOn") && obj["dependsOn"].isObject()) {
        const QJsonObject depsObj = obj["dependsOn"].toObject();
        for (auto it = depsObj.begin(); it != depsObj.end(); ++it) {
            const Result<FeatureDependency> dep
                = FeatureDependency::fromJson(it.key(), it.value().toObject());
            if (!dep)
                return ResultError(dep.error());
            f.dependsOn.push_back(*dep);
        }
    }

    const QJsonObject opts = obj.value("options").toObject();
    for (auto it = opts.begin(); it != opts.end(); ++it)
        f.options[it.key()] = FeatureOption::fromJson(it.value().toObject(), jsonStringToString);

    if (obj.contains("mounts") && obj["mounts"].isArray()) {
        const QJsonArray mountsArray = obj["mounts"].toArray();
        for (const QJsonValue &value : mountsArray) {
            const Result<std::variant<Mount, QString>> mount
                = Mount::fromJsonVariant(value, jsonStringToString);
            if (!mount)
                return ResultError(mount.error());
            f.mounts.push_back(*mount);
        }
    }

    f.init = obj.value("init").toBool();
    f.privileged = obj.value("privileged").toBool();
    f.deprecated = obj.value("deprecated").toBool();

    if (obj.contains("onCreateCommand"))
        f.onCreateCommand = parseCommand(obj["onCreateCommand"], jsonStringToString);
    if (obj.contains("updateContentCommand"))
        f.updateContentCommand = parseCommand(obj["updateContentCommand"], jsonStringToString);
    if (obj.contains("postCreateCommand"))
        f.postCreateCommand = parseCommand(obj["postCreateCommand"], jsonStringToString);
    if (obj.contains("postStartCommand"))
        f.postStartCommand = parseCommand(obj["postStartCommand"], jsonStringToString);
    if (obj.contains("postAttachCommand"))
        f.postAttachCommand = parseCommand(obj["postAttachCommand"], jsonStringToString);

    return f;
}
FeatureOption FeatureOption::fromJson(
    const QJsonObject &obj, const JsonStringToString &jsonStringToString)
{
    FeatureOption opt;
    opt.type = jsonStringToString(obj.value("type"));
    opt.description = jsonStringToString(obj.value("description"));
    opt.defaultValue = obj.value("default").toVariant();

    if (obj.contains("enum")) {
        const QJsonArray enumArray = obj.value("enum").toArray();
        for (const auto &e : enumArray)
            opt.enumValues.append(jsonStringToString(e));
    }
    if (obj.contains("proposals")) {
        const QJsonArray propArray = obj.value("proposals").toArray();
        for (const auto &p : propArray)
            opt.proposals.append(jsonStringToString(p));
    }

    return opt;
}

QJsonObject mergeCustomizations(QJsonObject left, const QJsonObject &right)
{
    for (QJsonObject::const_iterator it = right.constBegin(); it != right.constEnd(); ++it) {
        if (left.contains(it.key())) {
            QJsonValueRef leftMember = left[it.key()];
            if (leftMember.isObject() && it.value().isObject()) {
                leftMember = mergeCustomizations(leftMember.toObject(), it.value().toObject());
            } else if (leftMember.isArray() && it.value().isArray()) {
                QJsonArray leftArray = leftMember.toArray();
                const QJsonArray rightArray = it.value().toArray();
                for (const QJsonValue &value : rightArray)
                    leftArray.append(value);
                leftMember = leftArray;
            } else {
                leftMember = it.value();
            }
        } else {
            left.insert(it.key(), it.value());
        }
    }
    return left;
}

} // namespace DevContainer
