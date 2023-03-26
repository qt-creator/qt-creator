// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    signal clicked()

    property alias icon: button.buttonIcon
    property alias name: label.text
    property bool selected: false

    height: button.height
    width: button.width + label.width + contentRow.spacing + 6
    color: StudioTheme.Values.themeToolbarBackground
    radius: StudioTheme.Values.smallRadius

    state: "default"

    Row {
        id: contentRow
        spacing: 6

        HelperWidgets.AbstractButton {
            id: button
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.material_medium
            hover: mouseArea.containsMouse
            checked: root.selected
            checkable: true
            checkedInverted: true
            autoExclusive: true
        }

        Text {
            id: label
            height: StudioTheme.Values.statusbarButtonStyle.controlSize.height
            color: StudioTheme.Values.themeTextColor
            text: qsTr("Materials")
            font.pixelSize: StudioTheme.Values.baseFontSize
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse && !button.checked
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !button.checked
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackground_topToolbarHover
            }
        },
        State {
            name: "checked"
            when: !mouseArea.containsMouse && button.checked
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: label
                color: StudioTheme.Values.themeTextSelectedTextColor
            }
        },
        State {
            name: "hoverChecked"
            when: mouseArea.containsMouse && button.checked
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeInteractionHover
            }
            PropertyChanges {
                target: label
                color: StudioTheme.Values.themeTextSelectedTextColor
            }
        }
    ]
}
