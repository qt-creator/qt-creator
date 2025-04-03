// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets 2.0
import StudioTheme as StudioTheme
import "Texture" as Texture

Rectangle {
    id: itemPane

    width: 420
    height: 420
    color:  StudioTheme.Values.themePanelBackground

    // invoked from C++ to refresh material preview image
    function refreshPreview()
    {
        topSection.refreshPreview()
    }

    // Called from C++ to close context menu on focus out
    function closeContextMenu()
    {
        Controller.closeContextMenu()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Texture.ToolBar {
            Layout.fillWidth: true
        }

        PropertyEditorPane {
            Layout.fillWidth: true
            Layout.fillHeight: true

            headerComponent: Texture.TopSection {
                id: topSection
            }

            DynamicPropertiesSection {
                propertiesModel: PropertyEditorDynamicPropertiesModel {}
                visible: !hasMultiSelection
            }

            Loader {
                id: specificsTwo

                property string theSource: specificQmlData

                anchors.left: parent.left
                anchors.right: parent.right
                visible: specificsTwo.theSource !== ""
                sourceComponent: specificQmlComponent

                onTheSourceChanged: {
                    specificsTwo.active = false
                    specificsTwo.active = true
                }
            }

            Item {
                width: 1
                height: 10
                visible: specificsTwo.visible
            }

            Loader {
                id: specificsOne
                anchors.left: parent.left
                anchors.right: parent.right
                source: specificsUrl
            }

            TextureSection {
                width: itemPane.width
            }
        }
    }
}
