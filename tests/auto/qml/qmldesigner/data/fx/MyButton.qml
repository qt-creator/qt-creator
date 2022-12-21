// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 1.0

Rectangle {
    property string text: "test"
    property real myNumber: 10
    default property alias content: text.children
    width: 200
    height: 200
    Text {
        id: text
        anchors.fill: parent
    }

}
