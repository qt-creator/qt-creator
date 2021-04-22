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

#include "designermcumanager.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "designersettings.h"
#include "designdocument.h"

#include <qmljs/qmljssimplereader.h>

#include <utils/qtcassert.h>

namespace QmlDesigner {

static QString readProperty(const QString property, const QmlJS::SimpleReaderNode::Ptr &node)
{
    const QVariant propertyVar = node->property(property);

    if (!propertyVar.isNull() && propertyVar.isValid())
        return propertyVar.value<QString>();

    return {};
}

static QStringList readPropertyList(const QString &property, const QmlJS::SimpleReaderNode::Ptr &node)
{
    const QVariant propertyVar = node->property(property);

    if (!propertyVar.isNull() && propertyVar.isValid())
        return propertyVar.value<QStringList>();

    return {};
}

DesignerMcuManager &DesignerMcuManager::instance()
{
    static DesignerMcuManager instance;

    return instance;
}

QString DesignerMcuManager::mcuResourcesPath()
{
    return Core::ICore::resourcePath().pathAppended("qmldesigner/qt4mcu").toString();
}

bool DesignerMcuManager::isMCUProject() const
{
    QmlDesigner::DesignDocument *designDocument = QmlDesigner::QmlDesignerPlugin::instance()
                                                      ->documentManager().currentDesignDocument();
    if (designDocument)
        return designDocument->isQtForMCUsProject();

    return false;
}

void DesignerMcuManager::readMetadata()
{
    const QString mainMetadataFileName = "metadata.qml";

    m_defaultVersion = {};
    m_versionsList.clear();

    QmlJS::SimpleReader reader;
    const QmlJS::SimpleReaderNode::Ptr metadata =
            reader.readFile(mcuResourcesPath() + "/" + mainMetadataFileName);
    if (!metadata) {
        qWarning() << "Designer MCU metadata:" << reader.errors();
        return;
    }

    const QmlJS::SimpleReaderNode::List versions = metadata->children();

    if (versions.isEmpty()) {
        qWarning() << "Designer MCU metadata: metadata list is empty";
        return;
    }

    const QVariant defaultVersion = metadata->property("defaultVersion");
    if (!defaultVersion.isNull() && defaultVersion.isValid()) {
        for (const auto& version : versions) {
            Version newVersion;

            const QVariant vId = version->property("id");
            if (vId.isNull() || !vId.isValid())
                continue;

            const QVariant vName = version->property("name");
            if (!vName.isNull() && vName.isValid())
                newVersion.name = vName.value<QString>();
            else
                continue;

            const QVariant vPath = version->property("path");
            if (!vPath.isNull() && vPath.isValid())
                newVersion.fileName = vPath.value<QString>();
            else
                continue;

            m_versionsList.push_back(newVersion);

            if (vId == defaultVersion)
                m_defaultVersion = newVersion;
        }
    }
}

void DesignerMcuManager::readVersionData(const DesignerMcuManager::Version &version)
{
    m_currentVersion = {};
    m_bannedItems.clear();
    m_allowedImports.clear();
    m_bannedImports.clear();
    m_bannedProperties.clear();
    m_allowedItemProperties.clear();
    m_bannedComplexProperties.clear();

    QmlJS::SimpleReader reader;
    const QmlJS::SimpleReaderNode::Ptr versionData =
            reader.readFile(mcuResourcesPath() + "/" + version.fileName);
    if (!versionData) {
        qWarning() << "Designer MCU metadata:" << reader.errors();
        return;
    }

    const QmlJS::SimpleReaderNode::List info = versionData->children();

    if (info.isEmpty()) {
        qWarning() << "Designer MCU metadata: metadata list is empty";
        return;
    }

    for (const auto& child : info) {
        //handling specific item types:
        if (child->name() == "ComplexProperty") {
            if (child->propertyNames().contains("prefix")
                    && child->propertyNames().contains("bannedProperties")) {
                const QString complexPropPrefix(readProperty("prefix", child));
                const QStringList complexPropBans(readPropertyList("bannedProperties", child));

                if (!complexPropPrefix.isEmpty() && !complexPropBans.isEmpty())
                    m_bannedComplexProperties.insert(complexPropPrefix, complexPropBans);
            }

            continue;
        }

        //handling allowed properties:
        if (child->propertyNames().contains("allowedProperties")) {
            ItemProperties allowedProperties;

            const QVariant childrenPropertyVar = child->property("allowChildren");

            if (!childrenPropertyVar.isNull() && childrenPropertyVar.isValid())
                allowedProperties.allowChildren = childrenPropertyVar.toBool();

            allowedProperties.properties = readPropertyList("allowedProperties", child);

            if (!allowedProperties.properties.isEmpty())
                m_allowedItemProperties.insert(child->name(), allowedProperties);
        }

        //handling banned properties:
        const QStringList bannedProperties = readPropertyList("bannedProperties", child);

        m_bannedProperties.unite(QSet<QString>(bannedProperties.begin(), bannedProperties.end()));
    }

    const QList<QString> bannedItems = readPropertyList("bannedItems", versionData);

    m_bannedItems = QSet<QString>(bannedItems.begin(), bannedItems.end());
    m_allowedImports = readPropertyList("allowedImports", versionData);
    m_bannedImports = readPropertyList("bannedImports", versionData);
    m_currentVersion = version;
}

DesignerMcuManager::Version DesignerMcuManager::currentVersion() const
{
    return m_currentVersion;
}

DesignerMcuManager::Version DesignerMcuManager::defaultVersion() const
{
    return m_defaultVersion;
}

DesignerMcuManager::VersionsList DesignerMcuManager::versions() const
{
    return m_versionsList;
}

QSet<QString> DesignerMcuManager::bannedItems() const
{
    return m_bannedItems;
}

QSet<QString> DesignerMcuManager::bannedProperties() const
{
    return m_bannedProperties;
}

QStringList DesignerMcuManager::allowedImports() const
{
    return m_allowedImports;
}

QStringList DesignerMcuManager::bannedImports() const
{
    return m_bannedImports;
}

QHash<QString, DesignerMcuManager::ItemProperties> DesignerMcuManager::allowedItemProperties() const
{
    return m_allowedItemProperties;
}

QHash<QString, QStringList> DesignerMcuManager::bannedComplexProperties() const
{
    return m_bannedComplexProperties;
}

DesignerMcuManager::DesignerMcuManager()
{
    readMetadata();

    readVersionData(m_defaultVersion);
}

DesignerMcuManager::~DesignerMcuManager()
{

}

} // QmlDesigner
