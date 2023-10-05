// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectMakerBackend

Item {
    id: root

    property var draggedSec: null
    property var secsY: []
    property int moveFromIdx: 0
    property int moveToIdx: 0

    Column {
        id: col
        anchors.fill: parent
        spacing: 1

        EffectMakerTopBar {

        }

        EffectMakerPreview {
            mainRoot: root
        }

        Rectangle {
            width: parent.width
            height: StudioTheme.Values.toolbarHeight
            color: StudioTheme.Values.themeToolbarBackground

            EffectNodesComboBox {
                mainRoot: root

                anchors.verticalCenter: parent.verticalCenter
            }

            HelperWidgets.AbstractButton {
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter

                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.code
                tooltip: qsTr("Open Shader in Code Editor")

                onClicked: {} // TODO
            }
        }

        HelperWidgets.ScrollView {
            id: scrollView

            width: parent.width
            height: parent.height - y
            clip: true

            Column {
                width: scrollView.width
                spacing: 1

                Repeater {
                    id: repeater

                    width: root.width
                    model: EffectMakerBackend.effectMakerModel

                    onCountChanged: {
                        HelperWidgets.Controller.setCount("EffectMaker", repeater.count)
                    }

                    delegate: EffectCompositionNode {
                        width: root.width

                        Behavior on y {
                            PropertyAnimation {
                                duration: 300
                                easing.type: Easing.InOutQuad
                            }
                        }

                        onStartDrag: (section) => {
                            root.draggedSec = section
                            root.moveFromIdx = index

                            highlightBorder = true

                            root.secsY = []
                            for (let i = 0; i < repeater.count; ++i)
                                root.secsY[i] = repeater.itemAt(i).y
                        }

                        onStopDrag: {
                            if (root.moveFromIdx === root.moveToIdx)
                                root.draggedSec.y = root.secsY[root.moveFromIdx]
                            else
                                EffectMakerBackend.effectMakerModel.moveNode(root.moveFromIdx, root.moveToIdx)

                            highlightBorder = false
                            root.draggedSec = null
                        }
                    }
                } // Repeater

                Timer {
                    running: root.draggedSec
                    interval: 50
                    repeat: true

                    onTriggered: {
                        root.moveToIdx = root.moveFromIdx
                        for (let i = 0; i < repeater.count; ++i) {
                            let currItem = repeater.itemAt(i)
                            if (i > root.moveFromIdx) {
                                if (root.draggedSec.y > currItem.y + (currItem.height - root.draggedSec.height) * .5) {
                                    currItem.y = root.secsY[i] - root.draggedSec.height
                                    root.moveToIdx = i
                                } else {
                                    currItem.y = root.secsY[i]
                                }
                            } else if (i < root.moveFromIdx) {
                                if (root.draggedSec.y < currItem.y + (currItem.height - root.draggedSec.height) * .5) {
                                    currItem.y = root.secsY[i] + root.draggedSec.height
                                    root.moveToIdx = Math.min(root.moveToIdx, i)
                                } else {
                                    currItem.y = root.secsY[i]
                                }
                            }
                        }
                    }
                } // Timer
            } // Column
        } // ScrollView
    }

    Text {
        id: emptyText

        text: qsTr("Add an effect node to start")
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize

        x: scrollView.x + (scrollView.width - emptyText.width) * .5
        y: scrollView.y + scrollView.height * .5

        visible: EffectMakerBackend.effectMakerModel.isEmpty
    }
}
