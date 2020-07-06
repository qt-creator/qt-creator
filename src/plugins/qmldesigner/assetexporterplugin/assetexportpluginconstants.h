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

namespace QmlDesigner {
namespace Constants {

const char EXPORT_QML[] = "Designer.ExportPlugin.ExportQml";

const char TASK_CATEGORY_ASSET_EXPORT[] = "AssetExporter.Export";
const char UuidAuxTag[] = "uuid";

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

const char XPosTag[] = "x";
const char YPosTag[] = "y";
const char WidthTag[] = "width";
const char HeightTag[] = "height";


const char QmlIdTag[] = "qmlId";
const char ExportTypeTag[] = "exportType";
const char QmlPropertiesTag[] = "qmlProperties";
const char ImportsTag[] = "extraImports";
const char UuidTag[] = "uuid";
const char ClipTag[] = "clip";
const char AssetDataTag[] = "assetData";
const char AssetPathTag[] = "assetPath";
const char AssetBoundsTag[] = "assetBounds";
const char OpacityTag[] = "opacity";
const char TypeNameTag[] = "qmlType";

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
