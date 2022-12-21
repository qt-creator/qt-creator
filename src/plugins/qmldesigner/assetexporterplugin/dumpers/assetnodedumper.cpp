// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

