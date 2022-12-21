// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <auxiliarydata.h>

namespace QmlDesigner {
namespace Constants {

const char EXPORT_QML[] = "Designer.ExportPlugin.ExportQml";

const char TASK_CATEGORY_ASSET_EXPORT[] = "AssetExporter.Export";

//***************************************************************************
// Metadata tags
//***************************************************************************
// Plugin info tags
const char PluginInfoTag[] = "pluginInfo";
const char MetadataVersionTag[] = "metadataVersion";

const char DocumentInfoTag[] = "documentInfo";
const char DocumentNameTag[] = "name";

// Layer data tags
const char ArtboardListTag[] = "artboards";

const char NameTag[] = "name";

const char XPosTag[] = "x";
const char YPosTag[] = "y";
const char WidthTag[] = "width";
const char HeightTag[] = "height";

const char MetadataTag[] = "metadata";
const char ChildrenTag[] = "children";
const char CustomIdTag[] = "customId";
const char QmlIdTag[] = "qmlId";
const char ExportTypeTag[] = "exportType";
const char ExportTypeComponent[] = "component";
const char ExportTypeChild[] = "child";
const char QmlPropertiesTag[] = "qmlProperties";
const char ImportsTag[] = "extraImports";
const char UuidTag[] = "uuid";
const char ClipTag[] = "clip";
const char AssetDataTag[] = "assetData";
const char ReferenceAssetTag[] = "referenceAsset";
const char AssetPathTag[] = "assetPath";
const char AssetBoundsTag[] = "assetBounds";
const char OpacityTag[] = "opacity";
const char TypeNameTag[] = "typeName";
const char TypeIdTag[] = "typeId";

const char TextDetailsTag[] = "textDetails";
const char FontFamilyTag[] = "fontFamily";
const char FontSizeTag[] = "fontSize";
const char FontStyleTag[] = "fontStyle";
const char LetterSpacingTag[] = "kerning";
const char TextColorTag[] = "textColor";
const char TextContentTag[] = "contents";
const char IsMultilineTag[] = "multiline";
const char LineHeightTag[] = "lineHeight";
const char HAlignTag[] = "horizontalAlignment";
const char VAlignTag[] = "verticalAlignment";

}
}
