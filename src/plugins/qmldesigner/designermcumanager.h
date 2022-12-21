// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/icore.h>

#include <QString>
#include <QStringList>
#include <QSet>
#include <QHash>

namespace QmlDesigner {

class DesignerMcuManager
{
public:
    struct Version {
        QString name;
        QString fileName;
    };
    using VersionsList = QList<Version>;

    struct ItemProperties {
        QStringList properties;
        bool allowChildren = true;
    };

    static DesignerMcuManager& instance();

    static QString mcuResourcesPath();

    bool isMCUProject() const;

    void readMetadata();
    void readVersionData(const DesignerMcuManager::Version &version);

    DesignerMcuManager::Version currentVersion() const;
    DesignerMcuManager::Version defaultVersion() const;
    DesignerMcuManager::VersionsList versions() const;

    QSet<QString> bannedItems() const;
    QSet<QString> bannedProperties() const;

    QStringList allowedImports() const;
    QStringList bannedImports() const;

    QHash<QString, ItemProperties> allowedItemProperties() const;
    QHash<QString, QStringList> bannedComplexProperties() const;

    DesignerMcuManager(DesignerMcuManager const&) = delete;
    void operator=(DesignerMcuManager const&) = delete;

private:
    DesignerMcuManager();
    ~DesignerMcuManager();

private:
    DesignerMcuManager::Version m_currentVersion;
    DesignerMcuManager::Version m_defaultVersion;

    QSet<QString> m_bannedItems;
    QSet<QString> m_bannedProperties;
    QStringList m_allowedImports;
    QStringList m_bannedImports;
    QHash<QString, ItemProperties> m_allowedItemProperties;
    QHash<QString, QStringList> m_bannedComplexProperties;

    DesignerMcuManager::VersionsList m_versionsList;

};

} // namespace QmlDesigner
