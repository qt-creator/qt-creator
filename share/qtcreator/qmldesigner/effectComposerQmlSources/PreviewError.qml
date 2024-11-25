// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import EffectComposerBackend

Rectangle {
    id: root

    property bool showErrorDetails: false
    property var backendModel: EffectComposerBackend.effectComposerModel

    color: StudioTheme.Values.themePanelBackground
    border.color: StudioTheme.Values.themeControlOutline
    border.width: StudioTheme.Values.border

    HelperWidgets.ScrollView {
        id: scrollView

        anchors.fill: parent
        anchors.margins: 4
        visible: root.showErrorDetails

        clip: true

        TextEdit {
            width: scrollView.width
            font.pixelSize: StudioTheme.Values.myFontSize
            color: StudioTheme.Values.themeTextColor
            readOnly: true
            selectByMouse: true
            selectByKeyboard: true
            wrapMode: TextEdit.WordWrap
            text: root.backendModel ? root.backendModel.effectErrors : ""
        }
    }

    MouseArea {
        id: errorMouseArea

        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        anchors.fill: parent

        HelperWidgets.Button {
            width: 100
            height: 30
            text: qsTr("Show Less")
            visible: root.showErrorDetails
            opacity: errorMouseArea.containsMouse ? 1 : 0.3
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 4

            onClicked: root.showErrorDetails = false
        }
    }

    Column {
        anchors.centerIn: parent
        visible: !root.showErrorDetails
        spacing: 12
        Text {
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.bold: true
            font.pixelSize: StudioTheme.Values.baseFontSize
            color: StudioTheme.Values.themeTextColor
            text: qsTr("We are not able to create a preview of this effect.")
        }
        Row {
            height: 30
            spacing: 8
            anchors.horizontalCenter: parent.horizontalCenter
            Text {
                width: 150
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: StudioTheme.Values.themeTextColor
                elide: Text.ElideRight
                maximumLineCount: 1
                text: root.backendModel ? root.backendModel.effectErrors : ""
            }
            Text {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: StudioTheme.Values.baseFontSize
                font.underline: true
                color: StudioTheme.Values.themeTextColor
                linkColor: StudioTheme.Values.themeTextColor
                text: qsTr("<html><a href=\"#showmore\">Show More</a></html>")

                onLinkActivated: root.showErrorDetails = true
            }
        }
    }
}


