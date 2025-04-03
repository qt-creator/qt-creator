// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtCore
import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import "Material" as Material

Item {
    id: root

    width: 420
    height: 420

    // invoked from C++ to refresh material preview image
    signal refreshPreview()

    // Called from C++ to close context menu on focus out
    function closeContextMenu()
    {
        Controller.closeContextMenu()
    }

    Settings {
        id: settings

        property var topSection
        property bool dockMode
    }

    StudioControls.SplitView {
        id: splitView

        readonly property bool isHorizontal: splitView.orientation == Qt.Horizontal

        anchors.fill: parent
        orientation: splitView.width > 1000 ? Qt.Horizontal : Qt.Vertical
        clip: true

        Loader {
            id: leftSideView

            SplitView.fillWidth: leftSideView.visible
            SplitView.fillHeight: leftSideView.visible
            SplitView.minimumWidth: leftSideView.visible ? 300 : 0
            SplitView.minimumHeight: leftSideView.visible ? 300 : 0

            active: splitView.isHorizontal
            visible: leftSideView.active && leftSideView.item

            sourceComponent: hasMultiSelection ? blankPreview : preview
        }

        PropertyEditorPane {
            id: itemPane

            clip: true
            SplitView.fillWidth: !leftSideView.visible
            SplitView.fillHeight: true
            SplitView.minimumWidth: leftSideView.visible ? 400 : 0
            SplitView.maximumWidth: leftSideView.visible ? 800 : -1

            headerDocked: !leftSideView.visible && settings.dockMode

            headerComponent: Material.TopSection {
                id: topSection

                Component.onCompleted: topSection.restoreState(settings.topSection)
                Component.onDestruction: settings.topSection = topSection.saveState()
                previewComponent: preview
                showImage: !hasMultiSelection && !splitView.isHorizontal
            }

            toolbarComponent: Material.Toolbar {}

            DynamicPropertiesSection {
                propertiesModel: PropertyEditorDynamicPropertiesModel {}
                visible: !hasMultiSelection
            }

            Loader {
                id: specificsTwo

                property string theSource: specificQmlData

                width: itemPane.width
                visible: specificsTwo.theSource !== ""
                sourceComponent: specificQmlComponent

                onTheSourceChanged: {
                    specificsTwo.active = false
                    specificsTwo.active = true
                }
            }

            Item { // spacer
                width: 1
                height: 10
                visible: specificsTwo.visible
            }

            Loader {
                id: specificsOne
                width: itemPane.width
                source: specificsUrl
            }

            MaterialSection {
                width: itemPane.width
            }
        }
    }

    Component {
        id: preview

        Material.Preview {
            id: previewItem

            pinned: settings.dockMode
            showPinButton: !leftSideView.visible
            onPinnedChanged: settings.dockMode = previewItem.pinned

            Connections {
                target: root

                function onRefreshPreview() {
                    previewItem.refreshPreview()
                }
            }
        }
    }

    Component {
        id: blankPreview

        Rectangle {
            color: StudioTheme.Values.themePanelBackground
        }
    }
}
