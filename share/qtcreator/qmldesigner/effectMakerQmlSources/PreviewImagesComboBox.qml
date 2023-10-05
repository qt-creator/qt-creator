// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

StudioControls.ComboBox {
    id: root

    actionIndicatorVisible: false
    x: 5
    width: 60

    model: [selectedImage]

    // hide default popup
    popup.width: 0
    popup.height: 0

    required property Item mainRoot

    property var images: ["images/preview0.png",
                          "images/preview1.png",
                          "images/preview2.png",
                          "images/preview3.png",
                          "images/preview4.png"]
    property string selectedImage: images[0]

    Connections {
        target: root.popup

        function onAboutToShow() {
            var a = mainRoot.mapToGlobal(0, 0)
            var b = root.mapToItem(mainRoot, 0, 0)

            window.x = a.x + b.x + root.width - window.width
            window.y = a.y + b.y + root.height - 1

            window.show()
            window.requestActivate()
        }

        function onAboutToHide() {
            window.hide()
        }
    }

    contentItem: Item {
        width: 30
        height: 30

        Image {
            source: root.selectedImage
            fillMode: Image.PreserveAspectFit
            smooth: true
            anchors.fill: parent
            anchors.margins: 1
        }
    }

    Window {
        id: window

        width: col.width + 2 // 2: scrollView left and right 1px margins
        height: Math.min(800, Math.min(col.height + 2, Screen.height - y - 40)) // 40: some bottom margin to cover OS bottom toolbar
        flags: Qt.Dialog | Qt.FramelessWindowHint

        onActiveFocusItemChanged: {
            if (!window.activeFocusItem && !root.indicator.hover && root.popup.opened)
                root.popup.close()
        }

        Rectangle {
            anchors.fill: parent
            color: StudioTheme.Values.themePanelBackground
            border.color: StudioTheme.Values.themeInteraction
            border.width: 1

            HelperWidgets.ScrollView {
                anchors.fill: parent
                anchors.margins: 1
                clip: true

                Column {
                    id: col

                    onWidthChanged: {
                        // Needed to update on first window showing, as row.width only gets
                        // correct value after the window is shown, so first showing is off

                        var a = mainRoot.mapToGlobal(0, 0)
                        var b = root.mapToItem(mainRoot, 0, 0)

                        window.x = a.x + b.x + root.width - col.width
                    }

                    padding: 10
                    spacing: 10

                    Repeater {
                        model: root.images

                        Rectangle {
                            required property int index
                            required property var modelData

                            color: "transparent"
                            border.color: root.selectedImage === modelData ? StudioTheme.Values.themeInteraction
                                                                           : "transparent"

                            width: 200
                            height: 200

                            Image {
                                source: modelData
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                anchors.margins: 1
                            }

                            MouseArea {
                                anchors.fill: parent

                                onClicked: {
                                    root.selectedImage = root.images[index]
                                    root.popup.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
