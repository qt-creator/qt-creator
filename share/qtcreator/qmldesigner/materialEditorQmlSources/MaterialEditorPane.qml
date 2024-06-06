// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCore
import HelperWidgets

Item {
    id: root

    width: 420
    height: 420

    signal toolBarAction(int action)
    signal previewEnvChanged(string env)
    signal previewModelChanged(string model)

    // invoked from C++ to refresh material preview image
    function refreshPreview()
    {
        itemPane.headerItem.refreshPreview()
    }

    // Called from C++ to close context menu on focus out
    function closeContextMenu()
    {
        itemPane.headerItem.closeContextMenu()
        Controller.closeContextMenu()
    }

    // Called from C++ to initialize preview menu checkmarks
    function initPreviewData(env, model)
    {
        itemPane.headerItem.previewEnv = env
        itemPane.headerItem.previewModel = model
    }

    MaterialEditorToolBar {
        id: toolbar

        width: parent.width

        onToolBarAction: (action) => root.toolBarAction(action)
    }

    Settings {
        id: settings

        property var topSection
    }

    PropertyEditorPane {
        id: itemPane

        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom
        width: parent.width

        clip: true

        headerComponent: MaterialEditorTopSection {
            id: topSection
            onPreviewEnvChanged: root.previewEnvChanged(previewEnv)
            onPreviewModelChanged: root.previewModelChanged(previewModel)

            Component.onCompleted: topSection.restoreState(settings.topSection)
            Component.onDestruction: settings.topSection = topSection.saveState()
        }

        DynamicPropertiesSection {
            propertiesModel: MaterialEditorDynamicPropertiesModel {}
        }

        Loader {
            id: specificsTwo

            property string theSource: specificQmlData

            width: parent.width
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
            anchors.left: parent.left
            anchors.right: parent.right
            source: specificsUrl
        }
    }
}
