// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property string previewEnv
    property string previewModel
    property alias pinned: pinButton.checked
    property alias showPinButton: pinButton.visible

    property StudioTheme.ControlStyle buttonStyle: StudioTheme.ViewBarButtonStyle {
        // This is how you can override stuff from the control styles
        baseIconFontSize: StudioTheme.Values.bigIconFontSize
    }

    Connections {
        target: HelperWidgets.Controller

        function onCloseContextMenu() {
            root.closeContextMenu()
        }
    }

    implicitHeight: image.height

    clip: true
    color: "#000000"

    // Called from C++ to close context menu on focus out
    function closeContextMenu()
    {
        modelMenu.close()
        envMenu.close()
    }

    function refreshPreview()
    {
        image.source = ""
        image.source = "image://materialEditor/preview"
    }

    Image {
        id: image

        anchors.fill: parent
        fillMode: Image.PreserveAspectFit

        source: "image://materialEditor/preview"
        cache: false
        smooth: true

        sourceSize.width: image.width
        sourceSize.height: image.height

        Rectangle {
            id: toolbarRect

            radius: 10
            color: StudioTheme.Values.themeToolbarBackground
            width: optionsToolbar.width + 2 * toolbarRect.radius
            height: optionsToolbar.height + toolbarRect.radius
            anchors.left: parent.left
            anchors.leftMargin: -toolbarRect.radius
            anchors.verticalCenter: parent.verticalCenter

            Column {
                id: optionsToolbar

                spacing: 5
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: optionsToolbar.spacing

                HelperWidgets.AbstractButton {
                    id: pinButton

                    style: root.buttonStyle
                    buttonIcon: pinButton.checked ? StudioTheme.Constants.pin : StudioTheme.Constants.unpin
                    checkable: true
                }

                HelperWidgets.AbstractButton {
                    id: previewEnvMenuButton

                    style: root.buttonStyle
                    buttonIcon: StudioTheme.Constants.textures_medium
                    tooltip: qsTr("Select preview environment.")
                    onClicked: envMenu.popup()
                }

                HelperWidgets.AbstractButton {
                    id: previewModelMenuButton

                    style: root.buttonStyle
                    buttonIcon: StudioTheme.Constants.cube_medium
                    tooltip: qsTr("Select preview model.")
                    onClicked: modelMenu.popup()
                }
            }
        }
    }

    StudioControls.Menu {
        id: modelMenu
        closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

        ListModel {
            id: modelMenuModel
            ListElement {
                modelName: qsTr("Cone")
                modelStr: "#Cone"
            }
            ListElement {
                modelName: qsTr("Cube")
                modelStr: "#Cube"
            }
            ListElement {
                modelName: qsTr("Cylinder")
                modelStr: "#Cylinder"
            }
            ListElement {
                modelName: qsTr("Sphere")
                modelStr: "#Sphere"
            }
        }

        Repeater {
            model: modelMenuModel
            StudioControls.MenuItemWithIcon {
                text: modelName
                onClicked: {
                    // Force property change notifications to keep check mark when reselected
                    root.previewModel = ""
                    root.previewModel = modelStr
                }
                checkable: true
                checked: root.previewModel === modelStr
            }
        }
    }

    StudioControls.Menu {
        id: envMenu
        closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

        ListModel {
            id: envMenuModel
            ListElement {
                envName: qsTr("Basic")
                envStr: "Basic"
            }
            ListElement {
                envName: qsTr("Color")
                envStr: "Color"
            }
            ListElement {
                envName: qsTr("Studio")
                envStr: "SkyBox=preview_studio"
            }
            ListElement {
                envName: qsTr("Landscape")
                envStr: "SkyBox=preview_landscape"
            }
        }

        Repeater {
            model: envMenuModel
            StudioControls.MenuItemWithIcon {
                text: envName
                onClicked: {
                    // Force property change notifications to keep check mark when reselected
                    root.previewEnv = ""
                    root.previewEnv = envStr
                }
                checkable: true
                checked: root.previewEnv === envStr
            }
        }
    }
}
