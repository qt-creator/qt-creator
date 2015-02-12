/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

Item {
    id: selectionRangeDetails

    signal recenter
    signal close

    property string startTime
    property string endTime
    property string duration
    property bool showDuration

    width: Math.max(150, col.width + 25)
    height: col.height + 30

    // keep inside view
    Connections {
        target: selectionRangeDetails.parent
        onWidthChanged: fitInView();
        onHeightChanged: fitInView();
    }

    function detailedPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = Math.floor(t/1000);
        if (t<1000) return t+" μs";
        if (t<1e6) return (t/1000) + " ms";
        return (t/1e6) + " s";
    }

    function fitInView() {
        // don't reposition if it does not fit
        if (parent.width < width || parent.height < height)
            return;

        if (x + width > parent.width)
            x = parent.width - width;
        if (x < 0)
            x = 0;
        if (y + height > parent.height)
            y = parent.height - height;
        if (y < 0)
            y = 0;
    }

    // shadow
    BorderImage {
        property int px: 4
        source: "dialog_shadow.png"

        border {
            left: px; top: px
            right: px; bottom: px
        }
        width: parent.width + 2*px - 1
        height: parent.height
        x: -px + 1
        y: px + 1
    }

    // title bar
    Rectangle {
        width: parent.width
        height: 20
        color: "#4a64b8"
        radius: 5
        border.width: 1
        border.color: "#a0a0a0"
    }
    Item {
        width: parent.width+1
        height: 11
        y: 10
        clip: true
        Rectangle {
            width: parent.width-1
            height: 15
            y: -5
            color: "#4a64b8"
            border.width: 1
            border.color: "#a0a0a0"
        }
    }

    //title
    Text {
        id: typeTitle
        text: "  "+qsTr("Selection")
        font.bold: true
        height: 18
        y: 2
        verticalAlignment: Text.AlignVCenter
        width: parent.width
        color: "white"
        renderType: Text.NativeRendering
    }

    // Details area
    Rectangle {
        color: "white"
        width: parent.width
        height: col.height + 10
        y: 20
        border.width: 1
        border.color: "#a0a0a0"
        Grid {
            id: col
            x: 10
            y: 5
            spacing: 5
            columns: 2

            Repeater {
                model: [
                    qsTr("Start") + ":",
                    detailedPrintTime(startTime),
                    showDuration ? (qsTr("End") + ":") : "",
                    showDuration ? detailedPrintTime(endTime) : "",
                    showDuration ? (qsTr("Duration") + ":") : "",
                    showDuration ? detailedPrintTime(duration) : ""
                ]
                Detail {
                    isLabel: index % 2 === 0
                    text: modelData
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        drag.target: parent
        drag.minimumX: 0
        drag.maximumX: selectionRangeDetails.parent.width - width
        drag.minimumY: 0
        drag.maximumY: selectionRangeDetails.parent.height - height
        onClicked: selectionRangeDetails.recenter()
    }

    Text {
        id: closeIcon
        x: selectionRangeDetails.width - 14
        y: 4
        text:"X"
        color: "white"
        renderType: Text.NativeRendering
        MouseArea {
            anchors.fill: parent
            anchors.leftMargin: -8
            onClicked: selectionRangeDetails.close()
        }
    }
}
