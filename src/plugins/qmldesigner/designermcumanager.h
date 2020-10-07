/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
