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

Component {
    Rectangle {
        height: (delegateChooser.currentIndex == 1 && styleData.selected) ? 30 : 20
        Behavior on height{ NumberAnimation{} }

        color: styleData.selected ? "#448" : (styleData.alternate? "#eee" : "#fff")
        BorderImage{
            id: selected
            anchors.fill: parent
            source: "../images/selectedrow.png"
            visible: styleData.selected
            border{left:2; right:2; top:2; bottom:2}
            SequentialAnimation {
                running: true; loops: Animation.Infinite
                NumberAnimation { target:selected; property: "opacity"; to: 1.0; duration: 900}
                NumberAnimation { target:selected; property: "opacity"; to: 0.5; duration: 900}
            }
        }
    }
}
