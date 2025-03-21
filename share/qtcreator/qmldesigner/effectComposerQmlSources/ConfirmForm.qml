// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import HelperWidgets as HelperWidgets

Rectangle {
    id: root

    width: 200
    height: 30 + titleLabel.height + buttonRow.height
    border.width: 1
    border.color: StudioTheme.Values.themeControlOutline
    color: StudioTheme.Values.themeSectionHeadBackground

    property alias text: textLabel.text
    property alias acceptButtonLabel: acceptButton.text
    property alias cancelButtonLabel: cancelButton.text

    signal accepted()
    signal canceled()

    Column {
        id: column
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Item {
            id: titleLabel

            width: parent.width
            height: 50
            Text {
                id: textLabel
                anchors.centerIn: parent
                color: StudioTheme.Values.themeTextColor
                font.bold: true
                font.pixelSize: StudioTheme.Values.baseFontSize
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }

        Row {
            id: buttonRow

            width: acceptButton.width + buttonRow.spacing + cancelButton.width
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            height: 35

            HelperWidgets.Button {
                id: cancelButton
                width: 80
                height: 30
                text: qsTr("Cancel")
                padding: 4

                onClicked: {
                    root.canceled()
                    root.visible = false
                }
            }

            HelperWidgets.Button {
                id: acceptButton
                width: 80
                height: 30
                text: qsTr("Accept")
                padding: 4

                onClicked: {
                    root.accepted()
                    root.visible = false
                }
            }
        }
    }
}
