// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme

Item {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias caption: label.text
    property alias captionPixelSize: label.font.pixelSize
    property alias captionColor: header.color
    property alias captionTextColor: label.color
    property int leftPadding: 0
    property int rightPadding: 0
    property int topPadding: control.style.sectionHeadSpacerHeight
    property int bottomPadding: control.style.sectionHeadSpacerHeight
    property int animationDuration: 120
    property bool expanded: true
    property int collapsedBottomPadding: StudioTheme.Values.border

    clip: true

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        height: control.style.sectionHeadHeight
        color: control.style.section.head
        radius: StudioTheme.Values.smallRadius

        SectionLabel {
            id: arrow
            controlStyle: control.style
            width: control.style.smallIconSize.width
            height: control.style.smallIconSize.height
            text: StudioTheme.Constants.sectionToggle
            color: control.style.icon.idle
            renderType: Text.NativeRendering
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: control.style.smallIconFontSize
            font.family: StudioTheme.Constants.iconFont.family

            Behavior on rotation {
                NumberAnimation {
                    easing.type: Easing.OutCubic
                    duration: control.animationDuration
                }
            }
        }

        SectionLabel {
            id: label
            controlStyle: control.style
            anchors.verticalCenter: parent.verticalCenter
            color: control.style.text.idle
            x: 22
            width: header.width - label.x
            font.pixelSize: control.style.baseFontSize
            font.capitalization: Font.AllUppercase
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                control.expanded = !control.expanded
                if (!control.expanded) // TODO
                    control.forceActiveFocus()
            }
        }
    }

    default property alias __content: column.children

    readonly property alias contentItem: column

    implicitHeight: Math.round(column.height + header.height + topRow.height + bottomRow.height)

    Row {
        id: topRow
        height: control.topPadding
        anchors.top: header.bottom
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.leftMargin: control.leftPadding
        anchors.right: parent.right
        anchors.rightMargin: control.rightPadding
        anchors.top: topRow.bottom
    }

    Row {
        id: bottomRow
        height: control.bottomPadding
        anchors.top: column.bottom
    }

    Behavior on implicitHeight {
        NumberAnimation {
            easing.type: Easing.OutCubic
            duration: control.animationDuration
        }
    }

    states: [
        State {
            name: "Expanded"
            when: control.expanded
            PropertyChanges {
                target: arrow
                rotation: 0
            }
        },
        State {
            name: "Collapsed"
            when: !control.expanded
            PropertyChanges {
                target: control
                implicitHeight: header.height + control.collapsedBottomPadding
            }
            PropertyChanges {
                target: arrow
                rotation: -90
            }
        }
    ]
}
