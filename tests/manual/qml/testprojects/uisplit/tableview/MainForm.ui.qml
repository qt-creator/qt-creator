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
