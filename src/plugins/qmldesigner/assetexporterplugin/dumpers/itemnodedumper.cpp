// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemnodedumper.h"
#include "assetexportpluginconstants.h"
#include "assetexporter.h"
#include "componentexporter.h"

#include "qmlitemnode.h"
#include "annotation.h"

namespace  {
static QString capitalize(const QString &str)
{
    if (str.isEmpty())
        return {};
    QString tmp = str;
    tmp[0] = QChar(str[0]).toUpper().toLatin1();
    return tmp;
}
}

namespace QmlDesigner {
using namespace Constants;
ItemNodeDumper::ItemNodeDumper(const QByteArrayList &lineage,
                               const ModelNode &node) :
    NodeDumper(lineage, node)
{

}

bool QmlDesigner::ItemNodeDumper::isExportable() const
{
    return lineage().contains("QtQuick.Item");
}

QJsonObject QmlDesigner::ItemNodeDumper::json([[maybe_unused]] QmlDesigner::Component &component) const
{
    const QmlObjectNode &qmlObjectNode = objectNode();
    QJsonObject jsonObject;

    const QString qmlId = qmlObjectNode.id();
    QString name = m_node.simplifiedTypeName();
    if (!qmlId.isEmpty())
        name.append("_" + capitalize(qmlId));

    jsonObject.insert(NameTag, name);

    // Position relative to parent
    QmlItemNode itemNode = qmlObjectNode.toQmlItemNode();
    QPointF pos = itemNode.instancePosition();
    jsonObject.insert(XPosTag, pos.x());
    jsonObject.insert(YPosTag, pos.y());

    // size
    QSizeF size = itemNode.instanceSize();
    jsonObject.insert(WidthTag, size.width());
    jsonObject.insert(HeightTag, size.height());

    QJsonObject metadata;
    metadata.insert(QmlIdTag, qmlId);
    metadata.insert(UuidTag, uuid());
    metadata.insert(ExportTypeTag, ExportTypeChild);
    metadata.insert(TypeNameTag, QString::fromLatin1(m_node.type()));

    if (m_node.hasCustomId())
        metadata.insert(CustomIdTag, m_node.customId());

    QString typeId = component.exporter().componentUuid(m_node);
    if (!typeId.isEmpty())
        metadata.insert(TypeIdTag, typeId);

    jsonObject.insert(MetadataTag, metadata);
    return jsonObject;
}
}
