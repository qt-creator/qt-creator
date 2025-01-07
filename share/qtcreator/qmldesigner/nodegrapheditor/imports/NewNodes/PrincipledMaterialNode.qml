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

    readonly property QtQuick3D.PrincipledMaterial value: QtQuick3D.PrincipledMaterial {
    }

    metadata: QtObject {
        property var nodes: [
            {
                metadata: "output",
                text: "Output"
            },
            {
                metadata: "input",
                text: "Alpha Mode",
                sourceComponent: "materialAlphaMode",
                value: "alphaMode"
            },
            {
                metadata: "input",
                text: "Blend Mode",
                sourceComponent: "materialBlendMode",
                value: "blendMode"
            },
            {
                metadata: "input",
                text: "Lighting",
                sourceComponent: "materialLighting",
                value: "lighting"
            },
            {
                metadata: "color",
                text: "Base Color",
                bindings: values => {
                    value.baseColor = Qt.binding(() => {
                        return values.color;
                    });
                    value.baseColorChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    value.baseColorMap = Qt.binding(() => {
                        return values.map;
                    });
                    value.baseColorSingleChannelEnabled = Qt.binding(() => {
                        return values.singleChannelEnabled;
                    });
                }
            },
            {
                metadata: "metalness",
                text: "Metalness",
                bindings: values => {
                    value.metalness = Qt.binding(() => {
                        return values.metalness;
                    });
                    value.metalnessChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    value.metalnessMap = Qt.binding(() => {
                        return values.map;
                    });
                }
            },
            {
                metadata: "roughness",
                text: "Roughness",
                bindings: values => {
                    value.roughness = Qt.binding(() => {
                        return values.roughness;
                    });
                    value.roughnessChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    value.roughnessMap = Qt.binding(() => {
                        return values.map;
                    });
                }
            },
            {
                metadata: "normal",
                text: "Normal",
                bindings: values => {
                    value.normalMap = Qt.binding(() => {
                        return values.map;
                    });
                    value.normalStrength = Qt.binding(() => {
                        return values.strength;
                    });
                }
            },
            {
                metadata: "occlusion",
                text: "Occlusion",
                bindings: values => {
                    value.occlusionAmount = Qt.binding(() => {
                        return values.occlusion;
                    });
                    value.occlusionChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    value.occlusionMap = Qt.binding(() => {
                        return values.map;
                    });
                }
            },
            {
                metadata: "opacity",
                text: "Opacity",
                bindings: values => {
                    value.opacity = Qt.binding(() => {
                        return values.opacity;
                    });
                    value.opacityChannel = Qt.binding(() => {
                        return values.channel;
                    });
                    value.opacityMap = Qt.binding(() => {
                        return values.map;
                    });
                }
            },
        ]
    }
}
