// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 360
    height: 360
    Rectangle {
        width: 100; height: 100
        anchors.centerIn: parent
        color: "red"
    }
    Rectangle {
        width: 50; height: 50
        anchors.centerIn: parent
        color: "green"
    }
    Text {
        anchors.centerIn: parent
        text: "Check"
    }
}

