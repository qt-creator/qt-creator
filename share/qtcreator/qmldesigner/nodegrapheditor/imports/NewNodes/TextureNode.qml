// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick3D as QtQuick3D

import QuickQanava as Qan
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls

import NodeGraphEditorBackend

BaseGroup {
    id: root

    readonly property QtQuick3D.Texture value: QtQuick3D.Texture {
        sourceItem: image
    }

    metadata: QtObject {
        property var nodes: [
            {
                metadata: "output",
                text: "Output"
            },
            {
                metadata: "input",
                text: "Source",
                sourceComponent: "imageSource",
                value: "source"
            },
            {
                metadata: "input",
                text: "Scale.U",
                sourceComponent: "realSpinBox",
                value: "scaleU"
            },
            {
                metadata: "input",
                text: "Scale.V",
                sourceComponent: "realSpinBox",
                value: "scaleV"
            },
            {
                metadata: "input",
                text: "Flip.U",
                sourceComponent: "checkBox",
                value: "flipU"
            },
            {
                metadata: "input",
                text: "Flip.V",
                sourceComponent: "checkBox",
                value: "flipV"
            },
            {
                metadata: "input",
                text: "Auto Orientation",
                sourceComponent: "checkBox",
                value: "autoOrientation"
            },
            {
                metadata: "input",
                text: "Mapping Mode",
                sourceComponent: "textureMappingMode",
                value: "mappingMode"
            },
            {
                metadata: "input",
                text: "Tiling.U",
                sourceComponent: "textureTilingMode",
                value: "tilingModeHorizontal"
            },
            {
                metadata: "input",
                text: "Tiling.V",
                sourceComponent: "textureTilingMode",
                value: "tilingModeVertical"
            },
            {
                metadata: "input",
                text: "Mag Filter",
                sourceComponent: "textureFiltering",
                value: "magFilter"
            },
            {
                metadata: "input",
                text: "Min Filter",
                sourceComponent: "textureFiltering",
                value: "minFilter"
            },
            {
                metadata: "input",
                text: "Mip Filter",
                sourceComponent: "textureFiltering",
                value: "mipFilter"
            },
        ]
    }

    Image {
        id: image

        source: root.value.source
        visible: false
    }
}
