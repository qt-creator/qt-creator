// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Rectangle {
    id: itemPane
    width: 320
    height: 400
    color: StudioTheme.Values.themePanelBackground

    ColumnLayout {
        id: mainColumn
        anchors.fill: parent

        Controls.Label {
            text: qsTr("Select a component to see its properties.")
            font.pixelSize: StudioTheme.Values.myFontSize * 1.5
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap

            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Layout.margins: 20
        }
    }
}
