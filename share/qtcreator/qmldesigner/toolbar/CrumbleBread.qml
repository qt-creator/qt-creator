// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import StudioTheme 1.0 as StudioTheme

Item {
    id: root
    antialiasing: true

    property int modelSize

    /* Colors might come from Theme */
    property color idleBackgroundColor: StudioTheme.Values.themeControlBackground_toolbarIdle
    property color idleStrokeColor: StudioTheme.Values.controlOutline_toolbarIdle
    property color idleTextColor: StudioTheme.Values.themeTextColor
    property color hoverBackgroundColor: StudioTheme.Values.themeControlBackground_topToolbarHover
    property color hoverStrokeColor: StudioTheme.Values.controlOutline_toolbarHover
    property color activeColor: StudioTheme.Values.themeInteraction
    property color activeTextColor: StudioTheme.Values.themeTextSelectedTextColor

    property string tooltip

    property alias text: label.text
    property alias font: label.font
    property alias inset: backgroundPath.inset
    property alias strokeWidth: backgroundPath.strokeWidth

    property real textLeftMargin: 18
    property real textTopMargin: 6
    property real textRightMargin: 6
    property real textBottomMargin: 6

    readonly property int itemIndex: index
    readonly property bool isFirst: itemIndex === 0
    readonly property bool isLast: (itemIndex + 1) === modelSize

    signal clicked(int callIdx)

    width: 166
    height: 36

    Shape {
        id: backgroundShape
        anchors.fill: root

        antialiasing: root.antialiasing
        layer.enabled: antialiasing
        layer.smooth: antialiasing
        layer.samples: antialiasing ? 4 : 0

        ShapePath {
            id: backgroundPath

            joinStyle: ShapePath.MiterJoin
            fillColor: root.idleBackgroundColor
            strokeColor: root.idleStrokeColor
            strokeWidth: 1

            property real inset: 5
            property real rightOut: root.isLast ? 0 : root.inset
            property real leftIn: root.isFirst ? 0 : root.inset
            property real strokeOffset: backgroundPath.strokeWidth / 2

            property real topY: backgroundPath.strokeOffset
            property real bottomY: backgroundShape.height - backgroundPath.strokeOffset
            property real halfY: (backgroundPath.topY + backgroundPath.bottomY) / 2

            startX: backgroundPath.strokeOffset
            startY: backgroundPath.topY

            PathLine {
                x: backgroundShape.width - backgroundPath.strokeOffset - backgroundPath.rightOut
                y: backgroundPath.topY
            }
            PathLine {
                x: backgroundShape.width - backgroundPath.strokeOffset
                y: backgroundPath.halfY
            }
            PathLine {
                x: backgroundShape.width - backgroundPath.strokeOffset - backgroundPath.rightOut
                y: backgroundPath.bottomY
            }
            PathLine {
                x: backgroundPath.strokeOffset
                y: backgroundPath.bottomY
            }
            PathLine {
                x: backgroundPath.leftIn + backgroundPath.strokeOffset
                y: backgroundPath.halfY
            }
            PathLine {
                x: backgroundPath.strokeOffset
                y: backgroundPath.topY
            }
        }
    }

    Text {
        id: label

        anchors.fill: parent
        anchors.leftMargin: root.textLeftMargin
        anchors.topMargin: root.textTopMargin
        anchors.rightMargin: root.textRightMargin
        anchors.bottomMargin: root.textBottomMargin

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter

        color: root.idleTextColor

        wrapMode: Text.Wrap
        elide: Text.ElideRight
        maximumLineCount: 1

        text: "Crumble File"
        font.pixelSize: 12
        font.family: "SF Pro"

        ToolTip {
            id: toolTipBox
            text: root.tooltip
            visible: mouseArea.containsMouse
            delay: 1000

            MouseArea {
                anchors.fill: parent
                anchors.margins: -toolTipBox.padding
                onClicked: (mouseEvent) => mouseArea.clicked(mouseEvent)
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked(root.itemIndex)
    }

    states: [
        State {
            name: "idle"
            when: !mouseArea.containsMouse && !mouseArea.pressed && !root.isLast

            PropertyChanges {
                target: backgroundPath
                fillColor: root.idleBackgroundColor
                strokeColor: root.idleStrokeColor
            }
        },
        State {
            name: "active"
            when: root.isLast
            extend: "pressed"
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: backgroundPath
                fillColor: root.hoverBackgroundColor
                strokeColor: root.hoverStrokeColor
            }
        },
        State {
            name: "pressed"
            when: mouseArea.pressed

            PropertyChanges {
                target: backgroundPath
                strokeColor: root.activeColor
                fillColor: root.activeColor
            }

            PropertyChanges {
                target: label
                color: root.activeTextColor
            }
        }
    ]
}
