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

#include "itemnodedumper.h"
#include "assetexportpluginconstants.h"
#include "assetexporter.h"
#include "componentexporter.h"

#include "qmlitemnode.h"

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

QJsonObject QmlDesigner::ItemNodeDumper::json(QmlDesigner::Component &component) const
{
    Q_UNUSED(component);
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

    QString typeId = component.exporter().componentUuid(m_node);
    if (!typeId.isEmpty())
        metadata.insert(TypeIdTag, typeId);

    jsonObject.insert(MetadataTag, metadata);
    return jsonObject;
}
}
