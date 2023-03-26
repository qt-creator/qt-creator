// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

Component {
    id: delegate2
    Text {
        width: parent.width
        anchors.margins: 4
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        elide: styleData.elideMode
        text: styleData.value !== undefined  ? styleData.value : ""
        color: styleData.textColor
    }
}
