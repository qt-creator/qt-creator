// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.1

Rectangle {
    property string title

    property var clickedButton

    property var modality
    property var standardButtons

    property alias contentItem: contentArea

    property int maximumWidth: 0
    property int minimumWidth: 0

    property int maximumHeight: 0
    property int minimumHeight: 0

    Item {
        id: contentArea
        anchors.top: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.top
    }
}
