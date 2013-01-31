/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 1.0
import Monitor 1.0

Item {
    id: selectionRangeDetails

    property string startTime
    property string endTime
    property string duration
    property bool showDuration

    width: 170
    height: col.height + 30
    z: 1
    visible: false
    x: 200
    y: 125

    property int yoffset: root.scrollY
    onYoffsetChanged: {
        y = relativey + yoffset
        fitInView();
    }
    property int relativey : y - yoffset
    onYChanged: relativey = y - yoffset

    // keep inside view
    Connections {
        target: root
        onWidthChanged: fitInView();
        onCandidateHeightChanged: fitInView();
    }

    function fitInView() {
        // don't reposition if it does not fit
        if (root.width < width || root.candidateHeight < height)
            return;

        if (x + width > root.width)
            x = root.width - width;
        if (x < 0)
            x = 0;
        if (y + height - yoffset > root.candidateHeight)
            y = root.candidateHeight - height + yoffset;
        if (y < yoffset)
            y = yoffset;
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
    }

    // Details area
    Rectangle {
        color: "white"
        width: parent.width
        height: col.height + 10
        y: 20
        border.width: 1
        border.color: "#a0a0a0"
        Column {
            id: col
            x: 10
            y: 5
            Detail {
                label: qsTr("Start")
                content:  selectionRangeDetails.startTime
            }
            Detail {
                label: qsTr("End")
                visible: selectionRangeDetails.showDuration
                content:  selectionRangeDetails.endTime
            }
            Detail {
                label: qsTr("Duration")
                visible: selectionRangeDetails.showDuration
                content: selectionRangeDetails.duration
            }
        }
    }

    MouseArea {
        width: col.width + 45
        height: col.height + 30
        drag.target: parent
        drag.minimumX: 0
        drag.maximumX: root.width - parent.width
        drag.minimumY: yoffset
        drag.maximumY: root.candidateHeight - parent.height + yoffset
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
