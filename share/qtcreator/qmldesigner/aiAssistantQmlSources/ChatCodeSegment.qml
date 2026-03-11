// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    required property string code
    required property string language

    property bool adsFocus: false
    property bool collapsed: false

    implicitHeight: blockCol.implicitHeight

    ColumnLayout {
        id: blockCol

        anchors.left:  parent.left
        anchors.right: parent.right
        spacing: 0

        // Header bar
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: headerRow.implicitHeight + 10
            color:  StudioTheme.Values.themeSubPanelBackground
            radius: StudioTheme.Values.smallRadius

            RowLayout {
                id: headerRow

                anchors {
                    left:  parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: 8
                    rightMargin: 8
                }
                spacing: 6

                // Arrow icon
                Text {
                    color: StudioTheme.Values.themeTextColorDisabled
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.smallFontSize
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: StudioTheme.Constants.arrow_small
                    rotation: root.collapsed ? 0 : 90
                }

                Text {
                    Layout.fillWidth: true
                    text: root.language.length > 0 ? root.language : qsTr("code")
                    font.pixelSize: StudioTheme.Values.smallFontSize
                    color: StudioTheme.Values.themeTextColorDisabled
                    elide: Text.ElideRight
                }

                Text {
                    id: copyButton

                    text: qsTr("Copy")
                    font.pixelSize: StudioTheme.Values.smallFontSize
                    color: copyHover.containsMouse
                           ? StudioTheme.Values.themeTextColor
                           : StudioTheme.Values.themeTextColorDisabled

                    MouseArea {
                        id: copyHover

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape:  Qt.PointingHandCursor
                        onClicked: function(mouse) {
                            mouse.accepted = true
                            if (root.code.length === 0)
                                return
                            const t = Qt.createQmlObject(
                                'import QtQuick; TextEdit { visible: false }',
                                copyButton, "clipHelper")
                            t.text = root.code
                            t.selectAll()
                            t.copy()
                            t.destroy()
                            copyButton.text = qsTr("Copied!")
                            resetTimer.restart()
                        }
                    }

                    Timer {
                        id: resetTimer

                        interval: 1500
                        onTriggered: copyButton.text = qsTr("Copy")
                    }
                }
            }

            // Toggle collapse on header click; copy button sits above (z: -1 here)
            MouseArea {
                anchors.fill: parent
                z: -1
                onClicked: root.collapsed = !root.collapsed
            }
        }

        // Code body
        Rectangle {
            Layout.fillWidth: true
            visible: !root.collapsed
            implicitHeight: visible ? Math.min(codeText.implicitHeight + 16, 300) : 0
            clip: true
            color: Qt.darker(StudioTheme.Values.themeControlBackground_toolbarIdle, 1.08)
            radius: StudioTheme.Values.smallRadius

            Flickable {
                id: flickable

                anchors.fill: parent
                anchors.margins: 8
                contentWidth: codeText.implicitWidth
                contentHeight: codeText.implicitHeight
                clip: true
                flickableDirection: Flickable.HorizontalAndVerticalFlick
                boundsBehavior: Flickable.StopAtBounds

                HoverHandler { id: hoverHandler }

                ScrollBar.horizontal: StudioControls.TransientScrollBar {
                    id: horizontalScrollBar

                    style: StudioTheme.Values.viewStyle
                    parent: flickable

                    x: 0
                    width: flickable.availableWidth
                    orientation: Qt.Horizontal

                    show: (hoverHandler.hovered || flickable.focus || horizontalScrollBar.inUse || root.adsFocus)
                          && horizontalScrollBar.isNeeded
                }

                ScrollBar.vertical: StudioControls.TransientScrollBar {
                    id: verticalScrollBar

                    style: StudioTheme.Values.viewStyle
                    parent: flickable

                    y: 0
                    height: flickable.availableHeight
                    orientation: Qt.Vertical

                    show: (hoverHandler.hovered || flickable.focus || verticalScrollBar.inUse || root.adsFocus)
                          && verticalScrollBar.isNeeded
                }

                TextEdit {
                    id: codeText

                    text: root.code
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.NoWrap
                    readOnly: true
                    selectByKeyboard: true
                    font.family: "monospace"
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    color: StudioTheme.Values.themeTextColor
                }
            }
        }
    }
}
