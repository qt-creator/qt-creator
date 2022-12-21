// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme

Rectangle {
    id: root

    width: parent.width
    height: 50
    color: StudioTheme.Values.themeSectionHeadBackground

    property int currIndex: 0
    property alias tabsModel: repeater.model

    Row {
        spacing: 1

        Repeater {
            id: repeater

            ContentLibraryTabButton {
                height: root.height

                name: modelData.name
                icon: modelData.icon
                selected: root.currIndex === index
                onClicked: root.currIndex = index
            }
        }
    }
}
