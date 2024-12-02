// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property var rootEditor: shaderEditor
    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    color: StudioTheme.Values.themeToolbarBackground
    height: rowLayout.height

    RowLayout {
        id: rowLayout

        property real leftMargin: StudioTheme.Values.tabBarHorizontalMargin

        width: parent.width
        anchors.bottom: parent.bottom
        spacing: StudioTheme.Values.tabBarSpacing

        Item { // Empty item as a left margin
            implicitWidth: rowLayout.leftMargin - rowLayout.spacing
        }

        TabButton {
            text: qsTr("Fragment Shader")
            tabId: "FRAGMENT"
        }

        TabButton {
            text: qsTr("Vertex Shader")
            tabId: "VERTEX"
        }

        Item { // Spacer
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }
    }

    component TabButton: Label {
        id: tabButton

        required property string tabId
        readonly property bool selected: rootEditor.selectedShader === tabId

        Layout.alignment: Qt.AlignBottom

        font.pixelSize: StudioTheme.Values.mediumFont
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        leftPadding: StudioTheme.Values.tabButtonHorizontalPadding
        rightPadding: StudioTheme.Values.tabButtonHorizontalPadding
        topPadding: StudioTheme.Values.tabButtonVerticalPadding
        bottomPadding: StudioTheme.Values.tabButtonVerticalPadding

        color: {
            if (!tabButton.enabled)
                return root.style.text.disabled

            if (tabButton.selected)
                return root.style.text.selectedText

            return root.style.text.idle
        }

        background: Rectangle {
            color: {
                if (!tabButton.enabled)
                    return "transparent"

                if (tabItemMouseArea.containsMouse && tabButton.selected)
                    return root.style.interactionHover

                if (tabButton.selected)
                    return root.style.interaction

                if (tabItemMouseArea.containsMouse)
                    return root.style.background.hover

                return root.style.background.idle
            }

            border.width: 1
            border.color: {
                if (!tabButton.enabled)
                    return "transparent"

                if (tabButton.selected)
                    return root.style.border.interaction

                if (tabItemMouseArea.containsMouse)
                    return root.style.border.hover

                return root.style.border.idle
            }
        }

        MouseArea {
            id: tabItemMouseArea
            hoverEnabled: true
            anchors.fill: parent
            onClicked: rootEditor.selectedShader = tabButton.tabId
        }
    }
}
