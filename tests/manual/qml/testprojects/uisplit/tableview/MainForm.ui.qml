/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
