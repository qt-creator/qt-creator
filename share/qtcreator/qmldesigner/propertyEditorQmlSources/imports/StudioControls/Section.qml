// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Item {
    id: section
    property alias caption: label.text
    property alias captionPixelSize: label.font.pixelSize
    property alias captionColor: header.color
    property alias captionTextColor: label.color
    property int leftPadding: 8
    property int topPadding: 4
    property int rightPadding: 0
    property int animationDuration: 120
    property bool expanded: true

    clip: true

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        height: StudioTheme.Values.sectionHeadHeight
        color: StudioTheme.Values.themeSectionHeadBackground

        SectionLabel {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: StudioTheme.Values.themeTextColor
            x: 22
            font.pixelSize: StudioTheme.Values.myFontSize
            font.capitalization: Font.AllUppercase
        }

        SectionLabel {
            id: arrow
            width: StudioTheme.Values.spinControlIconSizeMulti
            height: StudioTheme.Values.spinControlIconSizeMulti
            text: StudioTheme.Constants.sectionToggle
            color: StudioTheme.Values.themeTextColor
            renderType: Text.NativeRendering
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
            font.family: StudioTheme.Constants.iconFont.family
            Behavior on rotation {
                NumberAnimation {
                    easing.type: Easing.OutCubic
                    duration: section.animationDuration
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                section.expanded = !section.expanded
                if (!section.expanded) // TODO
                    section.forceActiveFocus()
            }
        }
    }

    default property alias __content: column.children

    readonly property alias contentItem: column

    implicitHeight: Math.round(column.height + header.height + topRow.height + bottomRow.height)

    Row {
        id: topRow
        height: StudioTheme.Values.sectionHeadSpacerHeight
        anchors.top: header.bottom
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.leftMargin: leftPadding
        anchors.right: parent.right
        anchors.rightMargin: rightPadding
        anchors.top: topRow.bottom
    }

    Row {
        id: bottomRow
        height: StudioTheme.Values.sectionHeadSpacerHeight
        anchors.top: column.bottom
    }

    Behavior on implicitHeight {
        NumberAnimation {
            easing.type: Easing.OutCubic
            duration: section.animationDuration
        }
    }

    states: [
        State {
            name: "Expanded"
            when: section.expanded
            PropertyChanges {
                target: arrow
                rotation: 0
            }
        },
        State {
            name: "Collapsed"
            when: !section.expanded
            PropertyChanges {
                target: section
                implicitHeight: header.height
            }
            PropertyChanges {
                target: arrow
                rotation: -90
            }
        }
    ]
}
