// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

Column {

    width: 540
    height: 360
    property alias frame: frame

    property int margins: Qt.platform.os === "osx" ? 16 : 0

    MainTabView {
        id: frame

        height: parent.height - 34
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.margins: margins


        genteratedTabFrameVisible: frameCheckbox.checked
        genteratedTabHeaderVisible: headerCheckbox.checked
        genteratedTabSortIndicatorVisible: sortableCheckbox.checked
        genteratedTabAlternatingRowColors: alternateCheckbox.checked
    }

    Row {
        x: 12
        height: 34
        CheckBox{
            id: alternateCheckbox
            checked: true
            text: "Alternate"
            anchors.verticalCenter: parent.verticalCenter
        }
        CheckBox{
            id: sortableCheckbox
            checked: false
            text: "Sort indicator"
            anchors.verticalCenter: parent.verticalCenter
        }
        CheckBox{
            id: frameCheckbox
            checked: true
            text: "Frame"
            anchors.verticalCenter: parent.verticalCenter
        }
        CheckBox{
            id: headerCheckbox
            checked: true
            text: "Headers"
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
