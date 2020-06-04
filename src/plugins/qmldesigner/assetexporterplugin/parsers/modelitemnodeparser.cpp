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

#include "modelitemnodeparser.h"
#include "assetexportpluginconstants.h"

#include "modelnode.h"
#include "qmlitemnode.h"
#include "variantproperty.h"


namespace QmlDesigner {

ItemNodeParser::ItemNodeParser(const QByteArrayList &lineage, const ModelNode &node) :
    ModelNodeParser(lineage, node)
{

}

bool QmlDesigner::ItemNodeParser::isExportable() const
{
    return m_lineage.contains("QtQuick.Item");
}

QJsonObject QmlDesigner::ItemNodeParser::json() const
{
    // TODO parse other relevant properties i.e. dimensions etc
    QJsonObject jsonObject;
    jsonObject.insert("qmlid", m_node.id());
    QmlItemNode itemNode(m_node);

    // Position relative to parent
    QPointF pos = itemNode.instancePosition();
    jsonObject.insert("x", pos.x());
    jsonObject.insert("y", pos.y());

    // size
    QSizeF size = itemNode.instanceSize();
    jsonObject.insert("width", size.width());
    jsonObject.insert("height", size.height());

    return  jsonObject;
}
}
