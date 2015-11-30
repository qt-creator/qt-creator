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

Rectangle {

    property bool roundLeft: false
    property bool roundRight: false


    radius: roundLeft || roundRight ? 1 : 0
    gradient: Gradient {
        GradientStop {color: '#555' ; position: 0}
        GradientStop {color: '#444' ; position: 1}
    }

    border.width: roundLeft || roundRight ? 1 : 0
    border.color: "#2e2e2e"

    Rectangle {
        anchors.fill: parent
        visible: roundLeft && !roundRight
        anchors.leftMargin: 10
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        Component.onCompleted: {
            gradient = parent.gradient
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: roundRight && !roundLeft
        anchors.rightMargin: 10
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        Component.onCompleted: {
            gradient = parent.gradient
        }
    }

    Rectangle {
        color: "#2e2e2e"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        anchors.leftMargin: roundLeft ? 3 : 0
        anchors.rightMargin: roundRight ? 3 : 0
    }

    Rectangle {
        color: "#2e2e2e"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        anchors.leftMargin: roundLeft ? 2 : 0
        anchors.rightMargin: roundRight ? 2 : 0
    }

}
