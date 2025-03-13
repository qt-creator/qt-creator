// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property HelperWidgets.QmlTextureNodeProxy backend: textureNodeBackend

    readonly property string sourcePath: backendValues.source ? backendValues.source.valueToString : ""
    readonly property string previewPath: "image://qmldesigner_thumbnails/" + backend.resolveResourcePath(root.sourcePath)

    function refreshPreview()
    {
        texturePreview.source = ""
        texturePreview.source = root.previewPath
    }

    color: StudioTheme.Values.themePanelBackground
    implicitHeight: column.height

    Column {
        id: column

        Item { implicitWidth: 1; implicitHeight: 10 } // spacer

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
                source: root.previewPath
            }
        }

        HelperWidgets.Section {
            id: nameSection

            // Section with hidden header is used so properties are aligned with the other sections' properties
            hideHeader: true
            implicitWidth: root.width
            bottomPadding: StudioTheme.Values.sectionPadding * 2
            collapsible: false

            HelperWidgets.SectionLayout {
                HelperWidgets.PropertyLabel { text: qsTr("Name") }

                HelperWidgets.SecondColumnLayout {
                    HelperWidgets.Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                    HelperWidgets.LineEdit {
                        id: texName

                        implicitWidth: StudioTheme.Values.singleControlColumnWidth
                        width: StudioTheme.Values.singleControlColumnWidth
                        placeholderText: qsTr("Texture name")
                        showTranslateCheckBox: false
                        showExtendedFunctionButton: false

                        Timer {
                            running: true
                            interval: 0
                            onTriggered: texName.backendValue = backendValues.objectName
                            // backendValues.objectName is not available yet without the Timer
                        }

                        // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
                        validator: RegularExpressionValidator { regularExpression: /^(\w+\s)*\w+$/ }
                    }

                    HelperWidgets.ExpandingSpacer {}
                }
            }
        }
    }
}
