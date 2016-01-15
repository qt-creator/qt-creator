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
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1

Component {
TabViewStyle {
    tabOverlap: 16
    frameOverlap: 4
    tabsMovable: true

    frame: Rectangle {
        gradient: Gradient{
            GradientStop { color: "#e5e5e5" ; position: 0 }
            GradientStop { color: "#e0e0e0" ; position: 1 }
        }
        border.color: "#898989"
        Rectangle { anchors.fill: parent ; anchors.margins: 1 ; border.color: "white" ; color: "transparent" }
    }
    tab: Item {
        property int totalOverlap: tabOverlap * (control.count - 1)
        implicitWidth: Math.min ((styleData.availableWidth + totalOverlap)/control.count - 4, image.sourceSize.width)
        implicitHeight: image.sourceSize.height
        BorderImage {
            id: image
            anchors.fill: parent
            source: styleData.selected ? "../../images/tab_selected.png" : "../../images/tab.png"
            border.left: 30
            smooth: false
            border.right: 30
        }
        Text {
            text: styleData.title
            anchors.centerIn: parent
        }
    }
    leftCorner: Item { implicitWidth: 12 }
}
}
