// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import QtCreator.Tracing

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
        function onWidthChanged() { selectionRangeDetails.fitInView(); }
        function onHeightChanged() { selectionRangeDetails.fitInView(); }
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
        text: "  "+qsTranslate("QtC::Tracing", "Selection")
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
                    qsTranslate("QtC::Tracing", "Start") + ":",
                    TimeFormatter.format(selectionRangeDetails.startTime,
                                         selectionRangeDetails.referenceDuration),
                    (qsTranslate("QtC::Tracing", "End") + ":"),
                    TimeFormatter.format(selectionRangeDetails.endTime,
                                         selectionRangeDetails.referenceDuration),
                    (qsTranslate("QtC::Tracing", "Duration") + ":"),
                    TimeFormatter.format(selectionRangeDetails.duration,
                                         selectionRangeDetails.referenceDuration)
                ]

                model: selectionRangeDetails.showDuration ? 6 : 2
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
        ToolTip.text: qsTranslate("QtC::Tracing", "Close")
    }
}
