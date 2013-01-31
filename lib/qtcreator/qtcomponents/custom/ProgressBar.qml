/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0

Item {
    id: progressBar

    property real value: 0
    property real minimumValue: 0
    property real maximumValue: 1
    property bool indeterminate: false
    property bool containsMouse: mouseArea.containsMouse

    property int leftMargin: 0
    property int topMargin: 0
    property int rightMargin: 0
    property int bottomMargin: 0

    property int minimumWidth: 0
    property int minimumHeight: 0

    width: minimumWidth
    height: minimumHeight

    property Component background: null
    property Item backgroundItem: groove.item

    property color backgroundColor: syspal.base
    property color progressColor: syspal.highlight

    Loader {
        id: groove
        property alias indeterminate:progressBar.indeterminate
        property alias value:progressBar.value
        property alias maximumValue:progressBar.maximumValue
        property alias minimumValue:progressBar.minimumValue

        sourceComponent: background
        anchors.fill: parent
    }

    Item {
        anchors.fill: parent
        anchors.leftMargin: leftMargin
        anchors.rightMargin: rightMargin
        anchors.topMargin: topMargin
        anchors.bottomMargin: bottomMargin
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }
}
