// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/result.h>

#include <QString>
#include <QStringList>

namespace Android::Internal {

class AndroidManifestParser
{
public:
    struct ManifestData {
        QString iconName;
        bool hasIcon = false;
        QStringList permissions;
        bool hasDefaultPermissionsComment = false;
        bool hasDefaultFeaturesComment = false;
    };

    struct ModifyParams {
        bool shouldModifyApplication = false;
        QStringList applicationKeys;
        QStringList applicationValues;
        QStringList applicationKeysToRemove;

        bool shouldModifyPermissions = false;
        QSet<QString> permissionsToKeep;
        bool writeDefaultPermissionsComment = false;
        bool writeDefaultFeaturesComment = false;

        bool shouldModifyActivityMetaData = false;
        QString activityMetaDataName;
        QString activityMetaDataValue;
    };

    static Utils::Result<ManifestData> readManifest(const Utils::FilePath &manifestPath);
    static Utils::Result<void> processAndWriteManifest(const Utils::FilePath &manifestPath,
                                                       const ModifyParams &instructions);

};

Utils::Result<void> updateManifestApplicationAttribute(const Utils::FilePath &manifestPath,
                                                       const QString &attributeKey,
                                                       const QString &attributeValue);
Utils::Result<void> updateManifestPermissions(const Utils::FilePath &manifestPath,
                                              const QStringList &permissions,
                                              bool includeDefaultPermissions,
                                              bool includeDefaultFeatures);

Utils::Result<void> updateManifestActivityMetaData(const Utils::FilePath &manifestPath,
                                                   const QString &metaDataName,
                                                   const QString &metaDataValue);
Utils::Result<QString> readManifestActivityMetaData(const Utils::FilePath &manifestPath,
                                                    const QString &metaDataName);

} // namespace Android::Internal
