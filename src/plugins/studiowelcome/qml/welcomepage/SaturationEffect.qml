// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root

    property real desaturation: 1.0

    Rectangle {
        z: 10
        anchors.fill: parent
        color: "#2d2e30"
        anchors.margins: -16

        opacity: root.desaturation * 0.6
    }
}
