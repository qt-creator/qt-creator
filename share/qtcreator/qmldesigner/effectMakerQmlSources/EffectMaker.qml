// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectMakerBackend

Item {
    id: root

    Column {
        id: col
        anchors.fill: parent
        spacing: 5

        Rectangle {
            id: topHeader

            width: parent.width
            height: StudioTheme.Values.toolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            Row {
                // TODO
            }
        }

        Rectangle {
            id: previewHeader

            width: parent.width
            height: StudioTheme.Values.toolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            StudioControls.ComboBox {
                id: effectNodesComboBox

                actionIndicatorVisible: false
                x: 5
                width: parent.width - 50
                anchors.verticalCenter: parent.verticalCenter

                model: [qsTr("+ Add Effect")]

                // hide default popup
                popup.width: 0
                popup.height: 0

                Connections {
                    target: effectNodesComboBox.popup

                    function onAboutToShow() {
                        var a = root.mapToGlobal(0, 0)
                        var b = effectNodesComboBox.mapToItem(root, 0, 0)

                        effectNodesWindow.x = a.x + b.x + effectNodesComboBox.width - effectNodesWindow.width
                        effectNodesWindow.y = a.y + b.y + effectNodesComboBox.height - 1

                        effectNodesWindow.show()
                        effectNodesWindow.requestActivate()
                    }

                    function onAboutToHide() {
                        effectNodesWindow.hide()
                    }
                }

                Window {
                    id: effectNodesWindow

                    width: row.width + 2 // 2: scrollView left and right 1px margins
                    height: Math.min(800, Math.min(row.height + 2, Screen.height - y - 40)) // 40: some bottom margin to cover OS bottom toolbar
                    flags:  Qt.Popup | Qt.FramelessWindowHint

                    onActiveChanged: {
                        if (!active)
                            effectNodesComboBox.popup.close()
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: StudioTheme.Values.themePanelBackground
                        border.color: StudioTheme.Values.themeInteraction
                        border.width: 1

                        HelperWidgets.ScrollView {
                            anchors.fill: parent
                            anchors.margins: 1

                            Row {
                                id: row

                                onWidthChanged: {
                                    // Needed to update on first window showing, as row.width only gets
                                    // correct value after the window is shown, so first showing is off

                                    var a = root.mapToGlobal(0, 0)
                                    var b = effectNodesComboBox.mapToItem(root, 0, 0)

                                    effectNodesWindow.x = a.x + b.x + effectNodesComboBox.width - row.width
                                }

                                padding: 10
                                spacing: 10

                                Repeater {
                                    model: EffectMakerBackend.effectMakerNodesModel

                                    Column {
                                        spacing: 10

                                        Text {
                                            text: categoryName
                                            color: StudioTheme.Values.themeTextColor
                                            font.pointSize: StudioTheme.Values.baseFontSize
                                        }

                                        Item { width: 1; height: 5 } // spacer

                                        Repeater {
                                            model: categoryNodes

                                            Rectangle {
                                                width: 140
                                                height: 22

                                                color: mouseArea.containsMouse ? StudioTheme.Values.themeControlBackgroundInteraction
                                                                               : "transparent"

                                                MouseArea {
                                                    id: mouseArea

                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                }

                                                Row {
                                                    spacing: 5

                                                    IconImage {
                                                        id: nodeIcon

                                                        width: 22
                                                        height: 22

                                                        color: StudioTheme.Values.themeTextColor
                                                        source: modelData.nodeIcon
                                                    }

                                                    Text {
                                                        text: modelData.nodeName
                                                        color: StudioTheme.Values.themeTextColor
                                                        font.pointSize: StudioTheme.Values.smallFontSize
                                                        anchors.verticalCenter: nodeIcon.verticalCenter
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Image {
            id: previewImage

            // TODO
        }

        HelperWidgets.ScrollView {
            id: scrollView

            width: parent.width
            height: parent.height - y
            clip: true

            Behavior on contentY {
                id: contentYBehavior
                PropertyAnimation {
                    id: scrollViewAnim
                    easing.type: Easing.InOutQuad
                }
            }

            Column {
                Item {
                    width: scrollView.width
                    height: categories.height

                    Column {
                        Repeater {
                            id: categories
                            width: root.width
                            model: EffectMakerBackend.effectMakerModel

                            delegate: HelperWidgets.Section {
                                id: effectsSection
                                width: root.width
                                caption: model.categoryName
                                category: "EffectMaker"

                                property var effectList: model.effectNames

                                onExpandedChanged: {
                                    effects.visible = expanded // TODO: update
                                }

                                Repeater {
                                    id: effects
                                    model: effectList
                                    width: parent.width
                                    height: parent.height
                                    delegate: EffectNode {
                                        width: parent.width
                                        //height: StudioTheme.Values.checkIndicatorHeight * 2 // TODO: update or remove
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
