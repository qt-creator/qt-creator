// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCreator.Tracing

Item {
    id: timeDisplay

    property double windowStart
    property double rangeDuration

    property int textMargin: 5
    property int labelsHeight: Theme.toolBarHeight()
    property int fontSize: 8
    property int initialBlockLength: 120
    property double spacing: width / rangeDuration

    property double timePerBlock: Math.pow(2, Math.floor(Math.log(initialBlockLength / spacing) /
                                                       Math.LN2))

    property double alignedWindowStart: Math.round(windowStart - (windowStart % timePerBlock))
    property double pixelsPerBlock: timeDisplay.timePerBlock * timeDisplay.spacing
    property double pixelsPerSection: pixelsPerBlock / 5

    property int contentX
    property int offsetX: contentX + Math.round((windowStart % timePerBlock) * spacing)

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: timeDisplay.labelsHeight
        color: Theme.color(Theme.PanelStatusBarBackgroundColor)
    }

    Item {
        x: -(timeDisplay.offsetX % timeDisplay.pixelsPerBlock)
        y: 0
        id: timeDisplayArea

        property int firstBlock: timeDisplay.offsetX / timeDisplay.pixelsPerBlock
        property int offset: repeater.model > 0 ? repeater.model - (firstBlock % repeater.model)
                                                : 0;

        Repeater {
            id: repeater
            model: Math.max(0, Math.floor(timeDisplay.width / timeDisplay.initialBlockLength * 2
                                          + 2))

            Item {
                id: timeDisplayItem

                // Changing the text in text nodes is expensive. We minimize the number of changes
                // by rotating the nodes during scrolling.
                property int stableIndex: (index + timeDisplayArea.offset) % repeater.model

                height: timeDisplay.height
                y: 0
                x: width * stableIndex
                width: timeDisplay.pixelsPerBlock

                property double blockStartTime: (timeDisplayArea.firstBlock + stableIndex)
                                                * timeDisplay.timePerBlock
                                                + timeDisplay.alignedWindowStart

                TimelineText {
                    id: timeLabel

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: timeDisplay.labelsHeight

                    font.pixelSize: timeDisplay.fontSize
                    anchors.rightMargin: timeDisplay.textMargin
                    verticalAlignment: Text.AlignVCenter
                    text: TimeFormatter.format(timeDisplayItem.blockStartTime,
                                               timeDisplay.rangeDuration)
                    visible: width > 0
                    color: Theme.color(Theme.PanelTextColorLight)
                    elide: Text.ElideLeft
                }

                Row {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: timeLabel.bottom
                    anchors.bottom: parent.bottom

                    Repeater {
                        model: 4
                        Item {
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: timeDisplay.pixelsPerSection

                            Rectangle {
                                visible: timeDisplayItem.stableIndex !== 0
                                         || (-timeDisplayArea.x < parent.x + x)
                                color: Theme.color(Theme.Timeline_DividerColor)
                                width: 1
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                            }
                        }
                    }
                }

                Rectangle {
                    color: Theme.color(Theme.Timeline_DividerColor)
                    width: 1
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                }
            }
        }
    }
}
