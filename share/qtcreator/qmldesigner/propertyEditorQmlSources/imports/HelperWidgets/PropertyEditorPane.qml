// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme

Rectangle {
    id: itemPane

    width: 320
    height: 400
    color: StudioTheme.Values.themePanelBackground

    Component.onCompleted: Controller.mainScrollView = mainScrollView

    default property alias content: mainColumn.children

    // Called from C++ to close context menu on focus out
    function closeContextMenu() {
        Controller.closeContextMenu()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: forceActiveFocus()
    }

    HelperWidgets.ScrollView {
        id: mainScrollView
        //clip: true
        anchors.fill: parent

        interactive: !Controller.contextMenuOpened

        Column {
            id: mainColumn
            y: -1
            width: itemPane.width

            onWidthChanged: StudioTheme.Values.responsiveResize(itemPane.width)
            Component.onCompleted: StudioTheme.Values.responsiveResize(itemPane.width)
        }
    }
}
