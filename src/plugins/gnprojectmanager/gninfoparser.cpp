// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gninfoparser.h"

#include "gnutils.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace Utils;

namespace GNProjectManager::Internal::GNInfoParser {
static GNTarget extractTarget(const QString &label,
                              const QJsonObject &targetObj,
                              const FilePath &rootPath)
{
    GNTarget res;
    res.type = GNTarget::toType(targetObj["type"].toString());
    res.name = label;
    const int colonPos = label.indexOf(':');
    if (colonPos >= 0) {
        res.displayName = label.mid(colonPos + 1);
        res.path = label.left(colonPos);
    }

    const QJsonArray srcArray = targetObj["sources"].toArray();
    for (const auto &src : srcArray) {
        res.sources << resolveGNPath(src.toString(), rootPath);
    }

    const auto scriptIt = targetObj.constFind("script");
    if (scriptIt != targetObj.constEnd()) {
        res.sources << resolveGNPath(scriptIt->toString(), rootPath);
    }

    const QJsonArray outArray = targetObj["outputs"].toArray();
    for (const auto &out : outArray) {
        res.outputs << resolveGNPath(out.toString(), rootPath);
    }

    const QJsonArray cflagsArray = targetObj["cflags"].toArray();
    for (const auto &flag : cflagsArray) {
        res.cflags << flag.toString();
    }

    const QJsonArray cflagsccArray = targetObj["cflags_cc"].toArray();
    for (const auto &flag : cflagsccArray) {
        res.cflags << flag.toString();
    }

    const QJsonArray definesArray = targetObj["defines"].toArray();
    for (const auto &def : definesArray) {
        res.defines << def.toString();
    }

    const QJsonArray includesArray = targetObj["include_dirs"].toArray();
    for (const auto &inc : includesArray) {
        res.includes << resolveGNPath(inc.toString(), rootPath, true);
    }

    const QJsonArray depsArray = targetObj["deps"].toArray();
    for (const auto &dep : depsArray) {
        res.deps << dep.toString();
    }

    res.testonly = targetObj["testonly"].toBool(false);

    return res;
}

Result parse(const FilePath &projectJsonPath)
{
    Result result;

    QFile file(projectJsonPath.toFSPathString());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject())
        return result;

    const QJsonObject root = doc.object();

    // Parse build_settings
    const QJsonObject buildSettings = root["build_settings"].toObject();
    const QString rootPathStr = buildSettings["root_path"].toString();
    result.rootPath = FilePath::fromString(rootPathStr);

    const QString buildDirStr = buildSettings["build_dir"].toString();
    result.buildDir = FilePath::fromString(buildDirStr);

    // Parse gen_input_files (build system files)
    const QJsonArray genInputFiles = buildSettings["gen_input_files"].toArray();
    for (const auto &f : genInputFiles) {
        QString resolved = resolveGNPath(f.toString(), result.rootPath);
        result.buildSystemFiles << FilePath::fromString(resolved);
    }

    // Parse targets
    const QJsonObject targets = root["targets"].toObject();
    for (auto it = targets.constBegin(); it != targets.constEnd(); ++it) {
        result.targets.push_back(extractTarget(it.key(), it.value().toObject(), result.rootPath));
    }

    Utils::erase(result.targets, [](const GNTarget &target) {
        return target.type == GNTarget::Type::group
               || target.type == GNTarget::Type::action
               || target.type == GNTarget::Type::actionForEach
               || target.type == GNTarget::Type::bundleData
               || target.type == GNTarget::Type::createBundle
               || target.type == GNTarget::Type::copy
               || target.type == GNTarget::Type::generatedFile
               || target.type == GNTarget::Type::unknown;
    });
    return result;
}

} // namespace GNProjectManager::Internal::GNInfoParser
