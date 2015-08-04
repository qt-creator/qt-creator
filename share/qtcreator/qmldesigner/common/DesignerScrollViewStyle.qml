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
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1

ScrollViewStyle {
    property color scrollbarColor: "#444444"
    property color scrollbarBorderColor: "#333333"
    property color scrollBarHandleColor: "#656565"

    padding {left: 0; top: 0; right: 0; bottom: 0}

    scrollBarBackground: Rectangle {
        height: 10
        width: 10
        color: scrollbarColor
        border.width: 1
        border.color: scrollbarBorderColor
    }
    handle: Item {
        implicitWidth: 10
        implicitHeight: 10
        Rectangle {
            border.color: scrollbarBorderColor
            border.width: 1
            anchors.fill: parent
            color: scrollBarHandleColor
        }
    }

    decrementControl: Item {}
    incrementControl: Item {}
    corner: Item {}

    //Even if the platform style reports touch support a scrollview should not be flickable.
    Component.onCompleted: control.flickableItem.interactive = false
    transientScrollBars: false
}
