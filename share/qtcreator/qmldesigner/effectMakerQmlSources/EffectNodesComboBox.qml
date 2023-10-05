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
    width: parent.width - 50

    model: [qsTr("+ Add Effect")]

    // hide default popup
    popup.width: 0
    popup.height: 0

    required property Item mainRoot

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

    Window {
        id: window

        width: row.width + 2 // 2: scrollView left and right 1px margins
        height: Math.min(800, Math.min(row.height + 2, Screen.height - y - 40)) // 40: some bottom margin to cover OS bottom toolbar
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

                Row {
                    id: row

                    onWidthChanged: {
                        // Needed to update on first window showing, as row.width only gets
                        // correct value after the window is shown, so first showing is off

                        var a = mainRoot.mapToGlobal(0, 0)
                        var b = root.mapToItem(mainRoot, 0, 0)

                        window.x = a.x + b.x + root.width - row.width
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

                                EffectNode {
                                    onAddEffectNode: (nodeQenPath) => {
                                        EffectMakerBackend.rootView.addEffectNode(modelData.nodeQenPath)
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
}
