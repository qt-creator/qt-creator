/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
