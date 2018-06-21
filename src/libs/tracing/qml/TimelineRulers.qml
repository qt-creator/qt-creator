/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.0
import TimelineTheme 1.0

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
        height: scaleHeight

        onClicked: {
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
                        (x + contentX) * viewTimePerPixel + windowStart);
            }
        }
    }

    Repeater {
        model: rulersModel

        Item {
            id: ruler
            x: (timestamp - windowStart) / viewTimePerPixel - 1 - contentX
            y: 0
            width: 2
            height: rulersParent.height
            Rectangle {
                id: arrow
                height: scaleHeight
                width: scaleHeight
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
                width: scaleHeight / 4
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
