// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import PropertyToolBarAction

Rectangle {
    id: root

    property alias customToolbar: customToolbarLoader.sourceComponent

    signal toolBarAction(int action)

    color: StudioTheme.Values.themeToolbarBackground
    height: StudioTheme.Values.toolbarHeight

    Loader {
        id: customToolbarLoader

        anchors.left: parent.left
        anchors.right: lockButton.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

    HelperWidgets.AbstractButton {
        id: lockButton

        anchors.right: parent.right
        anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
        anchors.verticalCenter: parent.verticalCenter
        buttonIcon: lockButton.checked ? StudioTheme.Constants.lockOn : StudioTheme.Constants.lockOff
        checkable: true
        checked: isSelectionLocked ?? false
        enabled: !(hasMultiSelection ?? false)
        style: StudioTheme.Values.viewBarButtonStyle
        tooltip: qsTr("Lock current node")

        onClicked: root.toolBarAction(lockButton.checked ? ToolBarAction.SelectionLock : ToolBarAction.SelectionUnlock)
    }
}
