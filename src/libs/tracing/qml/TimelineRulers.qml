// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCreator.Tracing

Item {
    id: rulersParent
    property int scaleHeight
    property double viewTimePerPixel: 1
    property double contentX
    property double windowStart
    clip: true

    ListModel {
        id: rulersModel
    }

    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: rulersParent.scaleHeight

        onClicked: (mouse) => {
            rulersModel.append({
                timestamp: (mouse.x + contentX) * viewTimePerPixel + windowStart
            });
        }
    }

    Item {
        id: dragDummy
        property int index: -1
        onXChanged: {
            if (index >= 0) {
                rulersModel.setProperty(
                        index, "timestamp",
                        (x + rulersParent.contentX) * rulersParent.viewTimePerPixel
                            + rulersParent.windowStart);
            }
        }
    }

    Repeater {
        model: rulersModel

        Item {
            id: ruler
            x: (timestamp - rulersParent.windowStart) / rulersParent.viewTimePerPixel
                    - 1 - rulersParent.contentX
            y: 0
            width: 2
            height: rulersParent.height
            Rectangle {
                id: arrow
                height: rulersParent.scaleHeight
                width: rulersParent.scaleHeight
                rotation: 45
                anchors.verticalCenter: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                color: Theme.color(Theme.Timeline_HandleColor)
                MouseArea {
                    cursorShape: pressed ? Qt.DragMoveCursor : Qt.OpenHandCursor
                    anchors.fill: parent
                    drag.target: dragDummy
                    drag.axis: Drag.XAxis
                    drag.smoothed: false
                    onPressedChanged: {
                        if (!pressed) {
                            dragDummy.index = -1
                        } else {
                            dragDummy.x = ruler.x + 1
                            dragDummy.index = index
                        }
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.top: arrow.bottom
                anchors.bottom: parent.bottom
                width: 2
                color: Theme.color(Theme.Timeline_HandleColor)
            }

            Rectangle {
                anchors.top: arrow.bottom
                anchors.horizontalCenter: ruler.horizontalCenter
                width: rulersParent.scaleHeight / 4
                height: width
                color: Theme.color(Theme.Timeline_PanelBackgroundColor)

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width - 2
                    height: 1
                    color: Theme.color(Theme.Timeline_TextColor)
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: rulersModel.remove(index, 1)
                }
            }
        }
    }
}
