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
#include "componentexporter.h"
#include "assetexporter.h"
#include "assetexportpluginconstants.h"
#include "exportnotification.h"
#include "dumpers/nodedumper.h"

#include "model.h"
#include "nodeabstractproperty.h"
#include "nodemetainfo.h"
#include "qmlitemnode.h"
#include "rewriterview.h"

#include "utils/qtcassert.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPainter>

namespace {
Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.modelExporter", QtInfoMsg)

static QByteArrayList populateLineage(const QmlDesigner::ModelNode &node)
{
    QByteArrayList lineage;
    if (!node.isValid() || node.type().isEmpty())
        return {};

    for (auto &info : node.metaInfo().superClasses())
        lineage.append(info.typeName());

    return lineage;
}

}

namespace QmlDesigner {
using namespace Constants;

std::vector<std::unique_ptr<Internal::NodeDumperCreatorBase>> Component::m_readers;
Component::Component(AssetExporter &exporter, const ModelNode &rootNode):
    m_exporter(exporter),
    m_rootNode(rootNode)
{
    m_name = m_rootNode.id();
    if (m_name.isEmpty())
        m_name = QString::fromUtf8(m_rootNode.type());
}

QJsonObject Component::json() const
{
    return m_json;
}

AssetExporter &Component::exporter()
{
    return m_exporter;
}

void Component::exportComponent()
{
    QTC_ASSERT(m_rootNode.isValid(), return);
    m_json = nodeToJson(m_rootNode);
    // Change the export type to component
    QJsonObject metadata = m_json.value(MetadataTag).toObject();
    metadata.insert(ExportTypeTag, ExportTypeComponent);
    addReferenceAsset(metadata);
    m_json.insert(MetadataTag, metadata);
    addImports();
}

const QString &Component::name() const
{
    return m_name;
}

NodeDumper *Component::createNodeDumper(const ModelNode &node) const
{
    QByteArrayList lineage = populateLineage(node);
    std::unique_ptr<NodeDumper> reader;
    for (auto &dumperCreator: m_readers) {
        std::unique_ptr<NodeDumper> r(dumperCreator->instance(lineage, node));
        if (r->isExportable()) {
            if (reader) {
                if (reader->priority() < r->priority())
                    reader = std::move(r);
            } else {
                reader = std::move(r);
            }
        }
    }

    if (!reader)
        qCDebug(loggerInfo()) << "No dumper for node" << node;

    return reader.release();
}

QJsonObject Component::nodeToJson(const ModelNode &node)
{
    QJsonObject jsonObject;

    // Don't export States, Connection, Timeline etc nodes.
    if (!node.isSubclassOf("QtQuick.Item"))
        return {};

    std::unique_ptr<NodeDumper> dumper(createNodeDumper(node));
    if (dumper) {
        jsonObject = dumper->json(*this);
    } else {
        ExportNotification::addError(tr("Error exporting node %1. Cannot parse type %2.")
                                     .arg(node.id()).arg(QString::fromUtf8(node.type())));
    }

    QJsonArray children;
    for (const ModelNode &childnode : node.directSubModelNodes()) {
        const QJsonObject childJson = nodeToJson(childnode);
        if (!childJson.isEmpty())
            children.append(childJson);
    }

    if (!children.isEmpty())
        jsonObject.insert(ChildrenTag, children);

    return jsonObject;
}

void Component::addReferenceAsset(QJsonObject &metadataObject) const
{
    QPixmap refAsset = m_exporter.generateAsset(m_rootNode);
    stichChildrendAssets(m_rootNode, refAsset);
    Utils::FilePath refAssetPath = m_exporter.assetPath(m_rootNode, this, "_ref");
    m_exporter.exportAsset(refAsset, refAssetPath);
    QJsonObject assetData;
    if (metadataObject.contains(AssetDataTag))
        assetData = metadataObject[AssetDataTag].toObject();
    assetData.insert(ReferenceAssetTag, refAssetPath.toString());
    metadataObject.insert(AssetDataTag, assetData);
}

void Component::stichChildrendAssets(const ModelNode &node, QPixmap &parentPixmap) const
{
    if (!node.hasAnySubModelNodes())
        return;

    QPainter painter(&parentPixmap);
    for (const ModelNode &child : node.directSubModelNodes()) {
        QPixmap childPixmap = m_exporter.generateAsset(child);
        if (childPixmap.isNull())
            continue;
        stichChildrendAssets(child, childPixmap);
        QTransform cTransform = QmlObjectNode(child).toQmlItemNode().instanceTransform();
        painter.setTransform(cTransform);
        painter.drawPixmap(QPoint(0, 0), childPixmap);
    }
    painter.end();
}

void Component::addImports()
{
    QJsonArray importsArray;
    for (const Import &import : m_rootNode.model()->imports())
        importsArray.append(import.toString());

    if (!importsArray.empty())
        m_json.insert(Constants::ImportsTag, importsArray);
}


} // namespace QmlDesigner
