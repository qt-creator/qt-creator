// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AiAssistantBackend

Rectangle {
    id: root

    property alias closable: closeLoader.active
    property string source
    property var rootView: AiAssistantBackend.rootView

    signal closeRequest()
    signal clicked(event : MouseEvent)

    width: 60
    height: 40

    color: StudioTheme.Values.themeToolTipBackground

    Image {
        id: image

        fillMode: Image.PreserveAspectFit
        anchors.fill: parent
        source: root.rootView.fullImageUrl(root.source)
        cache: true
        asynchronous: true

        StudioControls.ToolTipArea {
            text: root.source
            anchors.fill: parent
            onClicked: (event) => root.clicked(event)
        }
    }

    Loader {
        id: closeLoader

        active: false
        anchors.right: parent.right
        anchors.top: parent.top

        width: 12
        height: 12

        sourceComponent: HelperWidgets.AbstractButton {
            id: removeImageButton
            objectName: "RemoveImageButton"

            anchors.fill: parent
            iconSize: 10

            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.close_small
            tooltip: qsTr("Remove the attached image.")

            onClicked: root.closeRequest()
        }
    }
}
