// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 60
    height: 60
    color: "green"
    property alias text: textItem.text

    Text {
        id: textItem
        anchors.centerIn: parent
    }
}
