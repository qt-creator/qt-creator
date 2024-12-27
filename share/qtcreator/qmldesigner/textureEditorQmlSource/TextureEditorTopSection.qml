// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme

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
