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

#include "assetnodedumper.h"
#include "assetexportpluginconstants.h"
#include "assetexporter.h"

#include "qmlitemnode.h"
#include "componentexporter.h"

#include "utils/fileutils.h"

#include <QPixmap>

namespace QmlDesigner {
using namespace Constants;
AssetNodeDumper::AssetNodeDumper(const QByteArrayList &lineage, const ModelNode &node) :
    ItemNodeDumper(lineage, node)
{

}

bool AssetNodeDumper::isExportable() const
{
    auto hasType =  [this](const QByteArray &type) {
        return lineage().contains(type);
    };
    return hasType("QtQuick.Image") || hasType("QtQuick.Rectangle");
}

QJsonObject AssetNodeDumper::json(Component &component) const
{
    QJsonObject jsonObject = ItemNodeDumper::json(component);

    AssetExporter &exporter = component.exporter();
    const Utils::FilePath assetPath = exporter.assetPath(m_node, &component);
    exporter.exportAsset(exporter.generateAsset(m_node), assetPath);

    QJsonObject assetData;
    assetData.insert(AssetPathTag, assetPath.toString());
    QJsonObject metadata = jsonObject.value(MetadataTag).toObject();
    metadata.insert(AssetDataTag, assetData);
    jsonObject.insert(MetadataTag, metadata);
    return jsonObject;
}
}

