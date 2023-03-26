// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    property alias text: textItem.text
    property alias color: rect.color
    property alias radius: rect.radius

    Rectangle {
        id: rect
        anchors.fill: parent
        color: "green"

        Text {
            id: textItem
            anchors.centerIn: parent
        }
    }
}
