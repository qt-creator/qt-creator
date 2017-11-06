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

import QtQuick 2.1
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import QtQuick.Controls.Private 1.0
import QtQuickDesignerTheme 1.0

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

            anchors.fill: parent
            visible: checked
            color: Theme.qmlDesignerBackgroundColorDarker()
        }

        RoundedPanel {
            roundLeft: isFirst()

            anchors.fill: parent
            visible: !checked
            color: Theme.qmlDesignerButtonColor()
        }
    }

    Image {
        id: image
        width: 16
        height: 16
        smooth: false
        anchors.centerIn: parent
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
