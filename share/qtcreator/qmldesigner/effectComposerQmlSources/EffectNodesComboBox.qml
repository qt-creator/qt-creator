// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

StudioControls.ComboBox {
    id: root

    actionIndicatorVisible: false

    model: [qsTr("+ Add Effect")]

    // hide default popup
    popup.width: 0
    popup.height: 0

    required property Item mainRoot

    readonly property int popupHeight: Math.min(800, row.height + 2)

    property bool nodeJustAdded: false

    function calculateWindowGeometry() {
        var globalPos = EffectComposerBackend.rootView.globalPos(mainRoot.mapFromItem(root, 0, 0))
        var screenRect = EffectComposerBackend.rootView.screenRect();

        window.width = row.width + 2 // 2: scrollView left and right 1px margins

        var newX = globalPos.x + root.width - window.width
        if (newX < screenRect.x)
            newX = globalPos.x

        var newY = Math.min(screenRect.y + screenRect.height,
                            Math.max(screenRect.y, globalPos.y + root.height - 1))

        // Check if we have more space above or below the control, and put control on that side,
        // unless we have enough room for maximum size popup under the control
        var newHeight
        var screenY = newY - screenRect.y
        if (screenRect.height - screenY > screenY || screenRect.height - screenY > root.popupHeight) {
            newHeight = Math.min(root.popupHeight, screenRect.height - screenY)
        } else {
            newHeight = Math.min(root.popupHeight, screenY - root.height)
            newY = newY - newHeight - root.height + 1
        }

        window.height = newHeight
        window.x = newX
        window.y = newY
    }

    Connections {
        target: root.popup

        function onAboutToShow() {
            EffectComposerBackend.rootView.updateCanBeAdded()
            root.calculateWindowGeometry()

            window.show()
            window.requestActivate()

            // Geometry can get corrupted by first show after screen change, so recalc it
            root.calculateWindowGeometry()
        }

        function onAboutToHide() {
            window.hide()
        }
    }

    Window {
        id: window

        flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

        onActiveFocusItemChanged: {
            if (!window.activeFocusItem && !root.hovered && root.popup.opened) {
                root.popup.close()
                confirmRemoveForm.visible = false
            }
        }

        Rectangle {
            anchors.fill: parent
            color: StudioTheme.Values.themePanelBackground
            border.color: StudioTheme.Values.themeInteraction
            border.width: 1
            focus: true

            HelperWidgets.ScrollView {
                anchors.fill: parent
                anchors.margins: 1
                enabled: !confirmRemoveForm.visible

                Row {
                    id: row

                    padding: 10
                    spacing: 10

                    Repeater {
                        model: EffectComposerBackend.effectComposerNodesModel

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
                                        EffectComposerBackend.rootView.addEffectNode(nodeQenPath)
                                        confirmRemoveForm.visible = false
                                        root.popup.close()
                                        root.nodeJustAdded = true
                                    }
                                    onRemoveEffectNodeFromLibrary: (nodeName) => {
                                        confirmRemoveForm.visible = true
                                        confirmRemoveForm.nodeName = nodeName
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ConfirmForm {
                id: confirmRemoveForm

                property string nodeName

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter

                width: 350
                height: 110
                visible: false
                text: qsTr("The node removal from library cannot be undone.\nAre you sure you want to remove node:\n'%1'?")
                        .arg(confirmRemoveForm.nodeName)
                acceptButtonLabel: qsTr("Remove")

                onAccepted: {
                    EffectComposerBackend.rootView.removeEffectNodeFromLibrary(confirmRemoveForm.nodeName)
                    confirmRemoveForm.visible = false
                }

                onCanceled: confirmRemoveForm.visible = false
            }


            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Escape && root.popup.opened) {
                    confirmRemoveForm.visible = false
                    root.popup.close()
                }
            }
        }
    }
}
