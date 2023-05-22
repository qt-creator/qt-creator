// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width
    spacing: 10
    padding: 10

    Text {
        text: qsTr("This is an instance of a component")
        anchors.horizontalCenter: parent.horizontalCenter
        width: 300
        horizontalAlignment: Text.AlignHCenter
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.myFontSize
    }

    RowLayout {
        Layout.fillWidth: true
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 0

        AbstractButton {
            implicitWidth: 180
            buttonIcon: qsTr("Edit Component")
            iconFont: StudioTheme.Constants.font

            onClicked: goIntoComponent()
        }
    }
}
