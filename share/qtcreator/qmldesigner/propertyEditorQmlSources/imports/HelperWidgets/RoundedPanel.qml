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
import QtQuickDesignerTheme 1.0

Rectangle {
    id: panel

    property bool roundLeft: true
    property bool roundRight: true


    /*
    radius: roundLeft || roundRight ? 1 : 0
    gradient: Gradient {
        GradientStop {color: '#555' ; position: 0}
        GradientStop {color: '#444' ; position: 1}
    }
    */

    border.width: roundLeft || roundRight ? 1 : 0
    color: Theme.qmlDesignerButtonColor()
    border.color: Theme.qmlDesignerBorderColor()

    Rectangle {
        anchors.fill: parent
        visible: roundLeft && !roundRight
        anchors.leftMargin: 10
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        color: panel.color
        Component.onCompleted: {
            //gradient = parent.gradient
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: roundRight && !roundLeft
        anchors.rightMargin: 10
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        color: panel.color
        Component.onCompleted: {
            //gradient = parent.gradient
        }
    }

    Rectangle {
        color: Theme.qmlDesignerBorderColor()
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        anchors.leftMargin: roundLeft ? 3 : 0
        anchors.rightMargin: roundRight ? 3 : 0
    }

    Rectangle {
        color: Theme.qmlDesignerBorderColor()
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        anchors.leftMargin: roundLeft ? 2 : 0
        anchors.rightMargin: roundRight ? 2 : 0
    }

}
