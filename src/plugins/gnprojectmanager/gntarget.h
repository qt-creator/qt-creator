// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QDir>
#include <QVariant>

namespace GNProjectManager::Internal {

struct GNTarget
{
    enum class Type {
        executable,
        sharedLibrary,
        staticLibrary,
        sourceSet,
        group,
        action,
        actionForEach,
        bundleData,
        createBundle,
        copy,
        generatedFile,
        unknown
    };

    Type type;
    QString name; // e.g. "//:hello"
    QString displayName;
    QString path;
    QStringList sources;
    QStringList outputs;
    QStringList cflags;
    QStringList defines;
    QStringList includes;
    QStringList deps;
    bool testonly;

    static Type toType(const QString &typeStr)
    {
        if (typeStr == "executable")
            return Type::executable;
        if (typeStr == "static_library")
            return Type::staticLibrary;
        if (typeStr == "shared_library")
            return Type::sharedLibrary;
        if (typeStr == "source_set")
            return Type::sourceSet;
        if (typeStr == "group")
            return Type::group;
        if (typeStr == "action")
            return Type::action;
        if (typeStr == "action_foreach")
            return Type::actionForEach;
        if (typeStr == "bundle_data")
            return Type::bundleData;
        if (typeStr == "create_bundle")
            return Type::createBundle;
        if (typeStr == "copy")
            return Type::copy;
        if (typeStr == "generated_file")
            return Type::generatedFile;
        return Type::unknown;
    }

    static QString typeToString(Type type)
    {
        switch (type) {
        case Type::executable:
            return QStringLiteral("executable");
        case Type::sharedLibrary:
            return QStringLiteral("shared_library");
        case Type::staticLibrary:
            return QStringLiteral("static_library");
        case Type::sourceSet:
            return QStringLiteral("source_set");
        case Type::group:
            return QStringLiteral("group");
        case Type::action:
            return QStringLiteral("action");
        case Type::actionForEach:
            return QStringLiteral("action_foreach");
        case Type::bundleData:
            return QStringLiteral("bundle_data");
        case Type::createBundle:
            return QStringLiteral("create_bundle");
        case Type::copy:
            return QStringLiteral("copy");
        case Type::generatedFile:
            return QStringLiteral("generated_file");
        case Type::unknown:
            return QStringLiteral("unknown");
        }
        return QStringLiteral("unknown");
    }
};

using GNTargetsList = std::vector<GNTarget>;

} // namespace GNProjectManager::Internal
