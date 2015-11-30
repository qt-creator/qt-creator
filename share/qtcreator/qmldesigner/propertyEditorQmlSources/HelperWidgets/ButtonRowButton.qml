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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

    property alias tooltip: toolTipArea.tooltip

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

    ToolTipArea {
        anchors.fill: parent
        id: toolTipArea
        anchors.leftMargin: leftPadding
        onClicked: {
            if (buttonRowButton.checked) {
                buttonRowButton.parent.__unCheckButton(index())
            } else {
                buttonRowButton.parent.__checkButton(index())
            }
            buttonRowButton.clicked()
       }

    }
}
