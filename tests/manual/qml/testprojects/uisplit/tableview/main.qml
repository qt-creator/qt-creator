// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

import "models"

Window {
    visible: true
    width: 538 + form.margins * 2
    height: 360 + form.margins * 2

    ToolBar {
        id: toolbar
        width: parent.width

        ListModel {
            id: delegatemenu
            ListElement { text: "Shiny delegate" }
            ListElement { text: "Scale selected" }
            ListElement { text: "Editable items" }
        }

        ComboBox {
            id: delegateChooser
            enabled: form.frame.currentIndex === 3 ? 1 : 0
            model: delegatemenu
            width: 150
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }

        CheckBox {
            id: enabledCheck
            text: "Enabled"
            checked: true
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    SystemPalette {id: syspal}
    color: syspal.window

    LargeModel {
        id: largeModel
    }

    MainForm {
        id: form
        anchors.top: toolbar.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom:  parent.bottom
        anchors.margins: 8
        frame.enabled: enabledCheck.checked
    }
}
