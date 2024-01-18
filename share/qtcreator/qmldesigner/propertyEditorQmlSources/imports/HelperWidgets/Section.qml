// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Item {
    id: section
    property alias caption: label.text
    property alias labelColor: label.color
    property alias sectionHeight: header.height
    property alias sectionBackgroundColor: header.color
    property alias sectionFontSize: label.font.pixelSize
    property alias showTopSeparator: topSeparator.visible
    property alias showArrow: arrow.visible
    property alias showLeftBorder: leftBorder.visible
    property alias showCloseButton: closeButton.visible
    property alias closeButtonToolTip: closeButton.tooltip
    property alias showEyeButton: eyeButton.visible
    property alias eyeButtonToolTip: eyeButton.tooltip
    property alias spacing: column.spacing
    property alias draggable: dragButton.visible
    property alias fillBackground: sectionBackground.visible
    property alias highlightBorder: sectionBorder.visible

    property int leftPadding: StudioTheme.Values.sectionLeftPadding
    property int rightPadding: 0
    property int topPadding: StudioTheme.Values.sectionHeadSpacerHeight
    property int bottomPadding: StudioTheme.Values.sectionHeadSpacerHeight

    property bool expanded: true
    property int level: 0
    property int levelShift: 10
    property bool hideHeader: false
    property bool collapsible: true
    property bool expandOnClick: true // if false, toggleExpand signal will be emitted instead
    property bool addTopPadding: true
    property bool addBottomPadding: true
    property bool dropEnabled: false
    property bool highlight: false
    property bool eyeEnabled: true // eye button enabled (on)

    property bool useDefaulContextMenu: true

    property string category: "properties"

    clip: true

    Connections {
        id: connection
        target: section
        enabled: section.useDefaulContextMenu
        function onShowContextMenu() { contextMenu.popup() }
    }

    Connections {
        target: Controller
        function onCollapseAll(cat) {
            if (collapsible && cat === section.category) {
                if (section.expandOnClick)
                    section.expanded = false
                else
                    section.collapse()
            }
        }
        function onExpandAll(cat) {
            if (cat === section.category) {
                if (section.expandOnClick)
                    section.expanded = true
                else
                    section.expand()
            }
        }
        function onCloseContextMenu() {
            contextMenu.close()
        }
        function onCountChanged(cat, count) {
            if (section.showEyeButton && cat === section.category)
                dragButton.enabled = count > 1
        }
    }

    signal drop(var drag)
    signal dropEnter(var drag)
    signal dropExit()
    signal showContextMenu()
    signal toggleExpand()
    signal expand()
    signal collapse()
    signal closeButtonClicked()
    signal eyeButtonClicked()
    signal startDrag(var section)
    signal stopDrag()

    DropArea {
        id: dropArea

        enabled: section.dropEnabled
        anchors.fill: parent

        onEntered: (drag) => section.dropEnter(drag)
        onDropped: (drag) => section.drop(drag)
        onExited: section.dropExit()
    }

    Rectangle {
        id: header
        height: section.hideHeader ? 0 : StudioTheme.Values.sectionHeadHeight
        visible: !section.hideHeader
        anchors.left: parent.left
        anchors.right: parent.right
        color: section.highlight ? StudioTheme.Values.themeInteraction
                                 : Qt.lighter(StudioTheme.Values.themeSectionHeadBackground, 1.0
                                              + (0.2 * section.level))

        Item {
            StudioControls.Menu {
                id: contextMenu

                StudioControls.MenuItem {
                    text: qsTr("Expand All")
                    onTriggered: Controller.expandAll(section.category)
                }

                StudioControls.MenuItem {
                    text: qsTr("Collapse All")
                    onTriggered: Controller.collapseAll(section.category)
                }

                onOpenedChanged: Controller.contextMenuOpened = contextMenu.opened
            }
        }

        Image {
            id: arrow
            width: 8
            height: 4
            source: "image://icons/down-arrow"
            anchors.left: parent.left
            anchors.leftMargin: 4 + (section.level * section.levelShift) + (section.draggable ? 20 : 0) + (section.showEyeButton ? 25 : 0)
            anchors.verticalCenter: parent.verticalCenter
        }

        Controls.Label {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: StudioTheme.Values.themeTextColor
            x: arrow.x + 18
            font.pixelSize: StudioTheme.Values.myFontSize
            font.capitalization: Font.AllUppercase
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    if (!section.collapsible && section.expanded)
                        return

                    transition.enabled = true
                    if (section.expandOnClick)
                        section.expanded = !section.expanded
                    else
                        section.toggleExpand()
                } else {
                    section.showContextMenu()
                }
            }
        }

        IconButton {
            id: closeButton

            icon: StudioTheme.Constants.closeCross
            buttonSize: 22
            iconScale: containsMouse ? 1.2 : 1
            transparentBg: true
            anchors.right: parent.right
            anchors.rightMargin: 10
            visible: false

            onClicked: root.closeButtonClicked()
        }

        IconButton {
            id: dragButton

            icon: StudioTheme.Constants.dragmarks
            buttonSize: 22
            iconScale: dragButton.enabled && dragButton.containsMouse ? 1.2 : 1
            transparentBg: true

            visible: false
            drag.target: dragButton.enabled ? section : null
            drag.axis: Drag.YAxis

            onPressed: {
                section.startDrag(section)

                section.z = ++section.parent.z // put the dragged section on top
            }

            onReleased: {
                section.stopDrag()
            }
        }

        IconButton {
            id: eyeButton

            anchors.left: dragButton.right

            icon: section.eyeEnabled ? StudioTheme.Constants.visible_small : StudioTheme.Constants.invisible_small
            buttonSize: 22
            iconScale: eyeButton.containsMouse ? 1.2 : 1
            transparentBg: true

            visible: false

            onClicked: {
                section.eyeEnabled = !section.eyeEnabled
                root.eyeButtonClicked()
            }
        }
    }

    Drag.active: dragButton.drag.active
    Drag.source: dragButton

    Rectangle {
        id: topSeparator
        height: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 5 + section.leftPadding
        anchors.leftMargin: 5 - section.leftPadding
        visible: false
        color: StudioTheme.Values.themeControlOutline
    }

    default property alias __content: column.children

    readonly property alias contentItem: column

    implicitHeight: Math.round(column.height + header.height + topSpacer.height + bottomSpacer.height)

    Rectangle {
        id: sectionBackground
        anchors.top: header.bottom
        width: section.width
        height: topSpacer.height + column.height + bottomSpacer.height
        color: StudioTheme.Values.themePanelBackground
        visible: false
    }

    Rectangle {
        id: sectionBorder
        anchors.fill: parent
        color: "transparent"
        border.color: StudioTheme.Values.themeInteraction
        border.width: 1
        visible: false
    }
    Item {
        id: topSpacer
        height: section.addTopPadding && column.height > 0 ? section.topPadding : 0
        anchors.top: header.bottom
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.leftMargin: section.leftPadding
        anchors.right: parent.right
        anchors.rightMargin: section.rightPadding
        anchors.top: topSpacer.bottom
    }

    Rectangle {
        id: leftBorder
        visible: false
        width: 1
        height: parent.height - bottomPadding
        color: header.color
    }

    Item {
        id: bottomSpacer
        height: section.addBottomPadding && column.height > 0 ? section.bottomPadding : 0
        anchors.top: column.bottom
    }

    states: [
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

    transitions: Transition {
        id: transition
        enabled: false
        NumberAnimation {
            properties: "implicitHeight,rotation"
            duration: 120
            easing.type: Easing.OutCubic
        }
    }
}
