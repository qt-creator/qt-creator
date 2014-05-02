/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.1
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import QtQuick.Controls.Private 1.0

Item {
    id: buttonRowButton
    property bool checked: false

    property bool roundLeftButton: true

    property alias iconSource: image.source

    signal clicked()

    property string tooltip: ""

    width: 24 + leftPadding
    height: 24

    property int leftPadding: 0

    function index() {
        for (var i = 0; i < parent.children.length; i++) {
            if (parent.children[i] === buttonRowButton)
                return i;
        }
        return -1;
    }

    function isFirst() {
        return index() === 0;
    }

    function isLast() {
        return index() === (parent.children.length - 1);
    }

    Item {
        anchors.fill: parent
        RoundedPanel {
            roundLeft: isFirst() && buttonRowButton.roundLeftButton
            roundRight: isLast()

            anchors.fill: parent
            z: checked ? 1 : 0

            gradient: Gradient {
                GradientStop {color: '#444' ; position: 0}
                GradientStop {color: '#333' ; position: 1}
            }
        }

        RoundedPanel {
            roundLeft: isFirst()
            roundRight: isLast()

            anchors.fill: parent
            z: !checked ? 1 : 0
        }
    }

    Image {
        id: image
        //source: iconSource
        anchors.centerIn: parent
        anchors.leftMargin: leftPadding
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.leftMargin: leftPadding
        onClicked: {
            if (buttonRowButton.checked) {
                buttonRowButton.parent.__unCheckButton(index())
            } else {
                buttonRowButton.parent.__checkButton(index())
            }
            buttonRowButton.clicked()
        }

        onExited: Tooltip.hideText()
        onCanceled: Tooltip.hideText()

        hoverEnabled: true

        Timer {
            interval: 1000
            running: mouseArea.containsMouse && tooltip.length
            onTriggered: Tooltip.showText(mouseArea, Qt.point(mouseArea.mouseX, mouseArea.mouseY), tooltip)
        }
    }
}
