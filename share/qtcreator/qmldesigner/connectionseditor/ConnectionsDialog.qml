// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import HelperWidgets as HelperWidgets

StudioControls.PopupDialog {
    id: root

    property alias backend: form.backend

    titleBar: Row {
        spacing: 30 // TODO
        anchors.fill: parent

        Text {
            color: StudioTheme.Values.themeTextColor
            text: qsTr("Target")
            font.pixelSize: StudioTheme.Values.myFontSize
            anchors.verticalCenter: parent.verticalCenter

            HelperWidgets.ToolTipArea {
                anchors.fill: parent
                tooltip: qsTr("Sets the Component that is connected to a <b>Signal</b>.")
            }
        }

        StudioControls.TopLevelComboBox {
            id: target
            style: StudioTheme.Values.connectionPopupControlStyle
            width: 180
            anchors.verticalCenter: parent.verticalCenter
            model: backend.signal.id.model ?? 0

            onActivated: backend.signal.id.activateIndex(target.currentIndex)
            property int currentTypeIndex: backend.signal.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: target.currentIndex = target.currentTypeIndex
        }
    }

    ConnectionsDialogForm {
        id: form

        Connections {
            target: root.backend
            function onPopupShouldClose() {
                root.close()
            }
            function onPopupShouldOpen() {
                Qt.callLater(root.showGlobal)
            }
        }
    }
}
