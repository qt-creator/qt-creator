// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts 1.0

Rectangle {
    id: tagBackground
    width: 84
    height: 15
    color: Constants.currentBrand
    visible: tagLabel.text.length

    property alias text: tagLabel.text

    Text {
        id: tagLabel
        color: Constants.darkActiveGlobalText
        text: qsTr("tag name")
        anchors.fill: parent
        font.pixelSize: 10
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        //font.weight: Font.ExtraLight
    }
}
