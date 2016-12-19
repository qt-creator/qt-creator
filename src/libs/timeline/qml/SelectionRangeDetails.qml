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
    id: selectionRangeDetails

    signal recenter
    signal close

    property double startTime
    property double endTime
    property double duration
    property double referenceDuration
    property bool showDuration
    property bool hasContents

    width: Math.max(150, col.width + 25)
    height: hasContents ? col.height + 30 : 0

    // keep inside view
    Connections {
        target: selectionRangeDetails.parent
        onWidthChanged: fitInView();
        onHeightChanged: fitInView();
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

    // title bar
    Rectangle {
        width: parent.width
        height: 20
        color: Theme.color(Theme.Timeline_PanelHeaderColor)
    }

    //title
    TimelineText {
        id: typeTitle
        text: "  "+qsTr("Selection")
        font.bold: true
        height: 20
        verticalAlignment: Text.AlignVCenter
        width: parent.width
        color: Theme.color(Theme.PanelTextColorLight)
    }

    // Details area
    Rectangle {
        color: Theme.color(Theme.Timeline_PanelBackgroundColor)
        width: parent.width
        height: col.height + 10
        y: 20
        Grid {
            id: col
            x: 10
            y: 5
            spacing: 5
            columns: 2

            Repeater {
                id: details
                property var contents: [
                    qsTr("Start") + ":",
                    TimeFormatter.format(startTime, referenceDuration),
                    (qsTr("End") + ":"),
                    TimeFormatter.format(endTime, referenceDuration),
                    (qsTr("Duration") + ":"),
                    TimeFormatter.format(duration, referenceDuration)
                ]

                model: showDuration ? 6 : 2
                Detail {
                    isLabel: index % 2 === 0
                    text: details.contents[index]
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

    ImageToolButton {
        id: closeIcon
        imageSource: "image://icons/close_window"
        anchors.right: selectionRangeDetails.right
        anchors.top: selectionRangeDetails.top
        implicitHeight: typeTitle.height
        onClicked: selectionRangeDetails.close()
    }
}
