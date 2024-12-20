// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

import Nodes as Nodes

StudioControls.Menu {
    id: contextMenu

    required property var graph
    property var inputsModel: []
    property var node

    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    onAboutToShow: {
        if (node && node.item)
            contextMenu.inputsModel = node.item.portsMetaData;
        else
            contextMenu.inputsModel = null;
    }

    StudioControls.MenuItem {
        text: qsTr("BaseColor")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.baseColor);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Metalness")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.metalness);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Roughness")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.roughness);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("CheckBox")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.checkBox);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Color")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.color);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("ComboBox")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.comboBox);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Material")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.material);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("RealSpinBox")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.realSpinBox);
        }
    }

    StudioControls.MenuItem {
        text: qsTr("Texture")

        onTriggered: () => {
            contextMenu.graph.insertNode(Nodes.Components.texture);
        }
    }

    StudioControls.MenuSeparator {
        height: implicitHeight
        style: StudioTheme.Values.controlStyle
    }

    StudioControls.Menu {
        enabled: node !== null
        title: "Pins"

        Repeater {
            id: pinRepeater

            model: contextMenu.inputsModel ? contextMenu.inputsModel.pin : null

            delegate: StudioControls.CheckBox {
                actionIndicator.visible: false
                checkState: (modelData.enabled === undefined || modelData.enabled === true) ? Qt.Checked : Qt.UnChecked
                text: "pin: " + modelData.name
                width: implicitWidth + StudioTheme.Values.toolbarHorizontalMargin

                onToggled: {
                    contextMenu.node.item.switchPin(modelData.id);
                    contextMenu.node.item.updatePinVisibility(modelData.id);
                }
            }
        }
    }
}
