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

import QtQuick 2.1
import TimelineTheme 1.0
import TimelineTimeFormatter 1.0

Item {
    id: timeDisplay

    property double windowStart
    property double rangeDuration

    property int textMargin: 5
    property int labelsHeight: 24
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
        id: row

        property int firstBlock: timeDisplay.offsetX / timeDisplay.pixelsPerBlock
        property int offset: repeater.model - (firstBlock % repeater.model);

        Repeater {
            id: repeater
            model: Math.floor(timeDisplay.width / timeDisplay.initialBlockLength * 2 + 2)

            Item {
                id: column

                // Changing the text in text nodes is expensive. We minimize the number of changes
                // by rotating the nodes during scrolling.
                property int stableIndex: (index + row.offset) % repeater.model

                height: timeDisplay.height
                y: 0
                x: width * stableIndex
                width: timeDisplay.pixelsPerBlock

                property double blockStartTime: (row.firstBlock + stableIndex)
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
                    text: TimeFormatter.format(column.blockStartTime, timeDisplay.rangeDuration)
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
                                visible: column.stableIndex !== 0 || (-row.x < parent.x + x)
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
