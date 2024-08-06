// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtCore
import HelperWidgets
import StudioControls 1.0 as StudioControls

Item {
    id: root

    property string __previewEnv
    property string __previewModel

    width: 420
    height: 420

    signal toolBarAction(int action)
    signal previewEnvChanged(string env)
    signal previewModelChanged(string model)

    // invoked from C++ to refresh material preview image
    signal refreshPreview()

    // Called from C++ to close context menu on focus out
    function closeContextMenu()
    {
        Controller.closeContextMenu()
    }

    // Called from C++ to initialize preview menu checkmarks
    function initPreviewData(env, model) {
        root.__previewEnv = env
        root.__previewModel = model
    }

    MaterialEditorToolBar {
        id: toolbar

        width: parent.width

        onToolBarAction: (action) => root.toolBarAction(action)
    }

    Settings {
        id: settings

        property var topSection
        property bool dockMode
    }

    StudioControls.SplitView {
        id: splitView

        readonly property bool isHorizontal: splitView.orientation == Qt.Horizontal

        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom
        width: parent.width
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

            sourceComponent: PreviewComponent {}
        }

        PropertyEditorPane {
            id: itemPane

            clip: true
            SplitView.fillWidth: !leftSideView.visible
            SplitView.fillHeight: true
            SplitView.minimumWidth: leftSideView.visible ? 400 : 0
            SplitView.maximumWidth: leftSideView.visible ? 800 : -1

            headerDocked: !leftSideView.visible && settings.dockMode

            headerComponent: MaterialEditorTopSection {
                id: topSection

                Component.onCompleted: topSection.restoreState(settings.topSection)
                Component.onDestruction: settings.topSection = topSection.saveState()
                previewComponent: PreviewComponent {}
                showImage: !splitView.isHorizontal
            }

            DynamicPropertiesSection {
                propertiesModel: MaterialEditorDynamicPropertiesModel {}
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
        }
    }

    component PreviewComponent : MaterialEditorPreview {
        id: previewItem

        onPreviewEnvChanged: root.previewEnvChanged(previewEnv)
        onPreviewModelChanged: root.previewModelChanged(previewModel)

        previewEnv: root.__previewEnv
        previewModel: root.__previewModel
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
