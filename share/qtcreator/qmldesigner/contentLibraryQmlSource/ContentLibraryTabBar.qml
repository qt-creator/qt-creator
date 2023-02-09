// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root
    property int currIndex: 0
    property alias tabsModel: repeater.model

    Row {
        leftPadding: 6
        rightPadding: 6
        spacing: 6
        Repeater {
            id: repeater

            ContentLibraryTabButton {
                required property int index
                required property var modelData
                name: modelData.name
                icon: modelData.icon
                selected: root.currIndex === index
                onClicked: root.currIndex = index
            }
        }
    }
}
