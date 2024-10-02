// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

Row {
    id: root

    property string currPath: ""
    property var pathSegments: []

    spacing: 5

    onCurrPathChanged: {
        let relativePath = root.currPath.slice(AssetsLibraryBackend.assetsModel.rootPath().length + 1)
        root.pathSegments = relativePath.split("/")
    }

    signal pathClicked(string path)

    Text {
        text: "root"
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize

        MouseArea {
            anchors.fill: parent

            onClicked: {
                var clickedPath = AssetsLibraryBackend.assetsModel.rootPath()
                root.pathClicked(clickedPath)
            }
        }
    }

    Repeater {
        model: root.pathSegments

        delegate: Row {
            spacing: 5

            Text {
                text: ">"
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.baseFontSize
            }

            Text {
                text: modelData
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.baseFontSize

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        var clickedPath = AssetsLibraryBackend.assetsModel.rootPath()
                                + "/" + pathSegments.slice(0, index + 1).join("/")

                        root.pathClicked(clickedPath)
                    }
                }
            }
        }
    }
}
