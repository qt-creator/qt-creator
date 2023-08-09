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

    property var effectMakerModel: EffectMakerBackend.effectMakerModel
    property var rootView: EffectMakerBackend.rootView

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
                // TODO: Filter row
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

                    width: 600
                    height: Math.min(400, Screen.height - y - 40) // TODO: window sizing will be refined
                    flags:  Qt.Popup | Qt.FramelessWindowHint

                    onActiveChanged: {
                        if (!active)
                            effectNodesComboBox.popup.close()
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: StudioTheme.Values.themePopupBackground
                        border.color: StudioTheme.Values.themeInteraction
                        border.width: 1
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
                            model: effectMakerModel

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
