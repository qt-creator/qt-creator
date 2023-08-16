// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Column {
    id: root

    width: parent.width

    Rectangle { // toolbar
        width: parent.width
        height: StudioTheme.Values.toolbarHeight
        color: StudioTheme.Values.themeToolbarBackground

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 5
            spacing: 5

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomOut_medium
                tooltip: qsTr("Zoom out")

                onClicked: {} // TODO
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomIn_medium
                tooltip: qsTr("Zoom In")

                onClicked: {} // TODO
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.cornersAll
                tooltip: qsTr("Zoom Fit")

                onClicked: {} // TODO
            }

            Column {
                Text {
                    text: "0.000s"
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: 10
                }

                Text {
                    text: "0000000"
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: 10
                }
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.toStartFrame_medium
                tooltip: qsTr("Restart Animation")

                onClicked: {} // TODO
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.topToolbar_runProject
                tooltip: qsTr("Play Animation")

                onClicked: {} // TODO
            }
        }
    }

    Rectangle { // preview image
        id: previewImage

        color: "#dddddd"
        width: parent.width
        height: 200

        Image {
            anchors.margins: 5
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit

            source: "images/qt_logo.png" // TODO: update image
        }
    }
}
