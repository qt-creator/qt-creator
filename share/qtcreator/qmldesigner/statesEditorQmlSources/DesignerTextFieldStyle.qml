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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1

TextFieldStyle {
    selectionColor: "#FFFFFF"
    selectedTextColor: "#000000"
    textColor: "#FFFFFF"
    placeholderTextColor: "#A0A0A0"

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: font.pixelSize + padding.top + padding.bottom
        border.color: "#222222"
        radius: 3
        gradient: Gradient {
            GradientStop {color: "#2c2c2c" ; position: 0}
            GradientStop {color: "#343434" ; position: 0.15}
            GradientStop {color: "#373737" ; position: 1.0}
        }
        Rectangle {
            border.color: highlightColor
            anchors.fill: parent
            anchors.margins: -1
            color: "#00000000" //Qt.transparent
            radius: 4
            opacity: 0.3
            visible: control.activeFocus
        }
    }
}
