// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Column {
    id: root

    signal toolBarAction(int action)

    function refreshPreview()
    {
        texturePreview.source = ""
        texturePreview.source = "image://qmldesigner_thumbnails/" + resolveResourcePath(backendValues.source.valueToString)
    }

    anchors.left: parent.left
    anchors.right: parent.right

    TextureEditorToolBar {
        width: root.width

        onToolBarAction: (action) => root.toolBarAction(action)
    }

    Item { width: 1; height: 10 } // spacer

    Rectangle {
        id: previewRect
        anchors.horizontalCenter: parent.horizontalCenter
        width: 152
        height: 152
        color: "#000000"

        Image {
            id: texturePreview
            asynchronous: true
            width: 150
            height: 150
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
            source: "image://qmldesigner_thumbnails/" + resolveResourcePath(backendValues.source.valueToString)
        }
    }
}
