// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1

ProgressBarStyle {
    background: Rectangle {
        implicitWidth: columnWidth
        implicitHeight: 24
        color: "#f0f0f0"
        border.color: "gray"
    }
    progress: Rectangle {
        color: "#ccc"
        border.color: "gray"
        Rectangle {
            color: "transparent"
            border.color: "#44ffffff"
            anchors.fill: parent
            anchors.margins: 1
        }
    }
}
