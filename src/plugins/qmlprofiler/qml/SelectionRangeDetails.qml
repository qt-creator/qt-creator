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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import Monitor 1.0

Item {
    id: selectionRangeDetails

    property string startTime
    property string endTime
    property string duration
    property bool showDuration

    width: Math.max(150, col.width + 25)
    height: col.height + 30
    z: 1
    visible: false
    x: 200
    y: 125

    // keep inside view
    Connections {
        target: root
        onWidthChanged: fitInView();
        onHeightChanged: fitInView();
    }

    function fitInView() {
        // don't reposition if it does not fit
        if (root.width < width || root.height < height)
            return;

        if (x + width > root.width)
            x = root.width - width;
        if (x < 0)
            x = 0;
        if (y + height > root.height)
            y = root.height - height;
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
                    startTime,
                    showDuration ? (qsTr("End") + ":") : "",
                    showDuration ? endTime : "",
                    showDuration ? (qsTr("Duration") + ":") : "",
                    showDuration ? duration : ""
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
        drag.maximumX: root.width - parent.width
        drag.minimumY: 0
        drag.maximumY: root.height - parent.height + 0
        onClicked: {
            if ((selectionRange.x < flick.contentX) ^ (selectionRange.x+selectionRange.width > flick.contentX + flick.width)) {
                root.recenter(selectionRange.startTime + selectionRange.duration/2);
            }
        }
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
            onClicked: {
                root.selectionRangeMode = false;
                root.updateRangeButton();
            }
        }
    }

}
