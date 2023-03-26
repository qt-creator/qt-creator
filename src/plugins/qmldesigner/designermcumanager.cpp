// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    const auto propertyVar = node->property(property);

    if (propertyVar.isValid())
        return propertyVar.value.value<QString>();

    return {};
}

static QStringList readPropertyList(const QString &property, const QmlJS::SimpleReaderNode::Ptr &node)
{
    const auto propertyVar = node->property(property);

    if (propertyVar.isValid())
        return propertyVar.value.value<QStringList>();

    return {};
}

DesignerMcuManager &DesignerMcuManager::instance()
{
    static DesignerMcuManager instance;

    return instance;
}

QString DesignerMcuManager::mcuResourcesPath()
{
    return Core::ICore::resourcePath("qmldesigner/qt4mcu").toString();
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

    const QmlJS::SimpleReaderNode::Property defaultVersion = metadata->property("defaultVersion");

    if (defaultVersion.isValid()) {
        for (const auto& version : versions) {
            Version newVersion;

            const auto vId = version->property("id");
            if (!vId.isValid())
                continue;

            const auto vName = version->property("name");
            if (vName.isValid())
                newVersion.name = vName.value.value<QString>();
            else
                continue;

            const auto vPath = version->property("path");
            if (vPath.isValid())
                newVersion.fileName = vPath.value.value<QString>();
            else
                continue;

            m_versionsList.push_back(newVersion);

            if (vId.value == defaultVersion.value)
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

            const auto childrenPropertyVar = child->property("allowChildren");

            if (childrenPropertyVar.isValid())
                allowedProperties.allowChildren = childrenPropertyVar.value.toBool();

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
