// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick3D as QtQuick3D

import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

import Nodes as Nodes
import NewNodes as NewNodes
import QuickQanava as Qan

StudioControls.Menu {
    id: contextMenu

    required property var graph
    property var inputsModel: []
    property point newPosition
    property var node

    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    onAboutToShow: {
        if (node && node.item)
            contextMenu.inputsModel = node.item.portsMetaData;
        else
            contextMenu.inputsModel = null;
    }

    StudioControls.Menu {
        title: "NewNodes.ProviderNodes"

        Component {
            id: providerNode

            NewNodes.ProviderNode {
            }
        }

        Repeater {
            model: [
                {
                    text: "Color",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["colorPicker"];
                        node.item.text = "Color provider";
                    }
                },
                {
                    text: "CheckBox",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["checkBox"];
                        node.item.text = "Boolean provider";
                    }
                },
                {
                    text: "Number",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["realSpinBox"];
                        node.item.text = "Number provider";
                    }
                },
                {
                    text: "Texture Source",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["imageSource"];
                        node.item.text = "Texture Source provider";
                    }
                },
                {
                    text: "Texture Mapping Mode",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["textureMappingMode"];
                        node.item.text = "Texture Mapping Mode provider";
                    }
                },
                {
                    text: "Texture Tiling Mode",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["textureTilingMode"];
                        node.item.text = "Texture Tiling Mode provider";
                    }
                },
                {
                    text: "Texture Filtering",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["textureFiltering"];
                        node.item.text = "Texture Filtering provider";
                    }
                },
                {
                    text: "Material Alpha Mode",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["materialAlphaMode"];
                        node.item.text = "Material Alpha Mode provider";
                    }
                },
                {
                    text: "Material Blend Mode",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["materialBlendMode"];
                        node.item.text = "Material Blend Mode provider";
                    }
                },
                {
                    text: "Material Channel",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["materialChannel"];
                        node.item.text = "Material Channel provider";
                    }
                },
                {
                    text: "Material Lighting",
                    action: () => {
                        const node = internal.createNode(providerNode);
                        node.item.sourceComponent = node.item.components["materialLighting"];
                        node.item.text = "Material Lighting provider";
                    }
                },
            ]

            delegate: StudioControls.MenuItem {
                text: modelData.text

                onTriggered: () => {
                    modelData.action();
                }
            }
        }
    }

    StudioControls.MenuItem {
        text: qsTr("NewNodes.TextureNode")

        onTriggered: () => {
            let gnode = contextMenu.graph.insertGroup(textureNode);
            gnode.item.x = contextMenu.newPosition.x;
            gnode.item.y = contextMenu.newPosition.y;
            gnode.label = "Texture";
        }

        Component {
            id: textureNode

            NewNodes.TextureNode {
            }
        }
    }

    StudioControls.MenuItem {
        text: qsTr("NewNodes.PrincipledMaterialNode")

        onTriggered: () => {
            let gnode = contextMenu.graph.insertGroup(principledMaterialNode);
            gnode.item.x = contextMenu.newPosition.x;
            gnode.item.y = contextMenu.newPosition.y;
            gnode.label = "PrincipledMaterial";
        }

        Component {
            id: principledMaterialNode

            NewNodes.PrincipledMaterialNode {
            }
        }
    }

    StudioControls.MenuItem {
        text: qsTr("NewNodes.MaterialViewerNode")

        onTriggered: () => {
            const node = internal.createNode(materialViewerNode);
        }

        Component {
            id: materialViewerNode

            NewNodes.MaterialViewerNode {
            }
        }
    }

    StudioControls.MenuItem {
        text: qsTr("BaseColor")

        onTriggered: () => {
            internal.createNode(Nodes.Components.baseColor);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Metalness")

        onTriggered: () => {
            internal.createNode(Nodes.Components.metalness);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Occlusion")

        onTriggered: () => {
            internal.createNode(Nodes.Components.occlusion);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Roughness")

        onTriggered: () => {
            internal.createNode(Nodes.Components.roughness);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("CheckBox")

        onTriggered: () => {
            internal.createNode(Nodes.Components.checkBox);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Color")

        onTriggered: () => {
            internal.createNode(Nodes.Components.color);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("ComboBox")

        onTriggered: () => {
            internal.createNode(Nodes.Components.comboBox);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Material")

        onTriggered: () => {
            internal.createNode(Nodes.Components.material);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("RealSpinBox")

        onTriggered: () => {
            internal.createNode(Nodes.Components.realSpinBox);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Texture")

        onTriggered: () => {
            internal.createNode(Nodes.Components.texture);
        }
    }

    StudioControls.MenuSeparator {
        height: implicitHeight
        style: StudioTheme.Values.controlStyle
    }

    StudioControls.Menu {
        enabled: node !== null && pinRepeater.count > 0
        title: "Pins"

        Repeater {
            id: pinRepeater

            model: contextMenu.inputsModel ? contextMenu.inputsModel.pin : null

            delegate: StudioControls.CheckBox {
                actionIndicator.visible: false
                checked: (modelData.enabled === undefined || modelData.enabled === true)
                text: "pin: " + modelData.name
                width: implicitWidth + StudioTheme.Values.toolbarHorizontalMargin

                onToggled: {
                    contextMenu.node.item.switchPin(modelData.id, checked);
                    contextMenu.node.item.updatePinVisibility(modelData.id);
                }
            }
        }
    }

    QtObject {
        id: internal

        function createNode(type) {
            const node = contextMenu.graph.insertNode(type);
            const nodeItem = node.item;
            nodeItem.x = contextMenu.newPosition.x;
            nodeItem.y = contextMenu.newPosition.y;
            return node;
        }
    }
}
