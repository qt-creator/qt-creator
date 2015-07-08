/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

Item {
    id: timeDisplay

    property double windowStart
    property double rangeDuration

    property int topBorderHeight: 2
    property int bottomBorderHeight: 1
    property int textMargin: 5
    property int labelsHeight: 24
    property int fontSize: 8
    property color color1: "#E6E6E6"
    property color color2: "white"
    property int initialBlockLength: 120
    property double spacing: width / rangeDuration

    property double timePerBlock: Math.pow(2, Math.floor(Math.log(initialBlockLength / spacing) /
                                                       Math.LN2))

    property double alignedWindowStart: Math.round(windowStart - (windowStart % timePerBlock))
    property double pixelsPerBlock: timeDisplay.timePerBlock * timeDisplay.spacing
    property double pixelsPerSection: pixelsPerBlock / 5

    property int contentX
    property int offsetX: contentX + Math.round((windowStart % timePerBlock) * spacing)

    readonly property var timeUnits: ["μs", "ms", "s"]
    function prettyPrintTime(t, rangeDuration) {
        if (rangeDuration < 1)
            return ""

        var round = 1;
        var barrier = 1;

        for (var i = 0; i < timeUnits.length; ++i) {
            barrier *= 1000;
            if (rangeDuration < barrier)
                round *= 1000;
            else if (rangeDuration < barrier * 10)
                round *= 100;
            else if (rangeDuration < barrier * 100)
                round *= 10;
            if (t < barrier * 1000)
                return Math.floor(t / (barrier / round)) / round + timeUnits[i];
        }

        t /= barrier;
        var m = Math.floor(t / 60);
        var s = Math.floor((t - m * 60) * round) / round;
        return m + "m" + s + "s";
    }

    Item {
        x: Math.floor(firstBlock * timeDisplay.pixelsPerBlock - timeDisplay.offsetX)
        y: 0
        id: row

        property int firstBlock: timeDisplay.offsetX / timeDisplay.pixelsPerBlock
        property int offset: firstBlock % repeater.model

        Repeater {
            id: repeater
            model: Math.floor(timeDisplay.width / timeDisplay.initialBlockLength * 2 + 2)

            Item {
                id: column

                // Changing the text in text nodes is expensive. We minimize the number of changes
                // by rotating the nodes during scrolling.
                property int stableIndex: row.offset > index ? repeater.model - row.offset + index :
                                                               index - row.offset
                height: timeDisplay.height
                y: 0
                x: width * stableIndex
                width: timeDisplay.pixelsPerBlock

                // Manually control this. We don't want it to happen when firstBlock
                // changes before stableIndex changes.
                onStableIndexChanged: block = row.firstBlock + stableIndex
                property int block: -1
                property double blockStartTime: block * timeDisplay.timePerBlock +
                                                timeDisplay.alignedWindowStart

                Rectangle {
                    id: timeLabel

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: timeDisplay.labelsHeight

                    color: (Math.round(column.block + timeDisplay.alignedWindowStart /
                                       timeDisplay.timePerBlock) % 2) ? color1 : color2;

                    Text {
                        id: labelText
                        renderType: Text.NativeRendering
                        font.pixelSize: timeDisplay.fontSize
                        font.family: "sans-serif"
                        anchors.fill: parent
                        anchors.leftMargin: timeDisplay.textMargin
                        anchors.bottomMargin: timeDisplay.textMargin
                        verticalAlignment: Text.AlignBottom
                        text: prettyPrintTime(column.blockStartTime, timeDisplay.rangeDuration)
                    }
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
                                color: "#CCCCCC"
                                width: 1
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                            }
                        }
                    }
                }

                Rectangle {
                    color: "#B0B0B0"
                    width: 1
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                }
            }
        }
    }

    Rectangle {
        height: topBorderHeight
        anchors.left: parent.left
        anchors.right: parent.right
        y: labelsHeight - topBorderHeight
        color: "#B0B0B0"
    }

    Rectangle {
        height: bottomBorderHeight
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: row.bottom
        color: "#B0B0B0"
    }
}
