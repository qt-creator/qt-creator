// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Item {
    id: root

    property bool editing: false
    property bool disableMoreMenu: false
    property bool showMoreMenu: true
    property alias editPropertyFormParent: editPropertyFormPlaceholder

    height: layout.implicitHeight + editPropertyFormPlaceholder.height + column.spacing
    visible: !uniformUseCustomValue

    signal reset()
    signal remove()
    signal edit()

    Component.onCompleted: {
        if (uniformType === "int") {
            if (uniformControlType === "channel")
                valueLoader.source = "ValueChannel.qml"
            else
                valueLoader.source = "ValueInt.qml"
        } else if (uniformType === "vec2") {
            valueLoader.source = "ValueVec2.qml"
        } else if (uniformType === "vec3") {
            valueLoader.source = "ValueVec3.qml"
        } else if (uniformType === "vec4") {
            valueLoader.source = "ValueVec4.qml"
        } else if (uniformType === "bool") {
            valueLoader.source = "ValueBool.qml"
        } else if (uniformType === "color") {
            valueLoader.source = "ValueColor.qml"
        } else if (uniformType === "sampler2D") {
            valueLoader.source = "ValueImage.qml"
        } else if (uniformType === "define") {
            if (uniformControlType === "int")
                valueLoader.source = "ValueInt.qml"
            else if (uniformControlType === "bool")
                valueLoader.source = "ValueBool.qml"
            else
                valueLoader.source = "ValueDefine.qml"
        } else {
            valueLoader.source = "ValueFloat.qml"
        }
    }

    Column {
        id: column
        anchors.fill: parent
        spacing: 5

        RowLayout {
            id: layout

            width: parent.width

            Text {
                id: textName

                text: (root.editing ? qsTr("[Editing] ") : "") + uniformDisplayName
                color: root.editing ? StudioTheme.Values.themeControlOutlineInteraction
                                    : StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.baseFontSize
                horizontalAlignment: Text.AlignRight
                Layout.preferredWidth: 140
                Layout.maximumWidth: Layout.preferredWidth
                Layout.minimumWidth: Layout.preferredWidth
                elide: Text.ElideRight

                HelperWidgets.ToolTipArea {
                    id: tooltipArea

                    anchors.fill: parent
                    tooltip: uniformDescription
                }
            }

            Item {
                Layout.preferredHeight: 30
                Layout.preferredWidth: 30
                Layout.maximumWidth: Layout.preferredWidth

                MouseArea {
                    id: mouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                }

                HelperWidgets.IconButton {
                    id: iconButton

                    buttonSize: 24
                    icon: StudioTheme.Constants.reload_medium
                    iconSize: 16
                    anchors.centerIn: parent
                    visible: mouseArea.containsMouse || iconButton.containsMouse
                    tooltip: qsTr("Reset value")
                    onClicked: root.reset()
                }
            }

            Loader {
                id: valueLoader
                Layout.fillWidth: true
            }

            Item {
                Layout.preferredHeight: 30
                Layout.preferredWidth: 30
                Layout.maximumWidth: Layout.preferredWidth

                HelperWidgets.IconButton {
                    id: moreButton
                    buttonSize: 24
                    icon: StudioTheme.Constants.more_medium
                    iconSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    tooltip: root.disableMoreMenu ? qsTr("Additional actions disabled while editing existing property.")
                                                  : qsTr("Access additional property actions.")
                    enabled: !root.disableMoreMenu
                    visible: root.showMoreMenu

                    onClicked: menuLoader.show()
                }

                Loader {
                    id: menuLoader

                    active: false

                    function show() {
                        menuLoader.active = true
                        item.popup()
                    }

                    sourceComponent: Component {
                        StudioControls.Menu {
                            id: menu

                            StudioControls.MenuItem {
                                text: qsTr("Edit")
                                onTriggered: root.edit()
                            }

                            StudioControls.MenuItem {
                                text: qsTr("Remove")
                                onTriggered: root.remove()
                            }
                        }
                    }
                }
            }
        }
        Item {
            id: editPropertyFormPlaceholder
            width: parent.width
            height: childrenRect.height
        }
    }
}
