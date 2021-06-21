/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    property var pointA: Qt.vector2d()
    property var pointB: Qt.vector2d()

    property bool linked: false

    property var middle: {
        var ab = root.pointB.minus(root.pointA) // B - A
        return root.pointA.plus(ab.normalized().times(ab.length() * 0.5))
    }

    property var position: {
        // Calculate the middle point between A and B
        var ab = root.pointB.minus(root.pointA) // B - A
        var midAB = root.pointA.plus(ab.normalized().times(ab.length() * 0.5))
        var perpendicularAB = Qt.vector2d(ab.y, -ab.x)
        return midAB.plus(perpendicularAB.normalized().times(8.0 * StudioTheme.Values.scaleFactor))
    }

    color: "transparent"
    border.color: "transparent"

    x: root.position.x - (StudioTheme.Values.height * 0.5)
    y: root.position.y - (StudioTheme.Values.height * 0.5)

    implicitWidth: StudioTheme.Values.height
    implicitHeight: StudioTheme.Values.height

    transformOrigin: Item.Center

    T.Label {
        id: icon
        anchors.fill: parent
        text: root.linked ? StudioTheme.Constants.linked
                          : StudioTheme.Constants.unLinked
        visible: true
        color: "grey"
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: 4.0 * StudioTheme.Values.scaleFactor
        hoverEnabled: true
        onPressed: root.linked = !root.linked
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse
            PropertyChanges {
                target: icon
                color: "grey"//StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse
            PropertyChanges {
                target: icon
                color: "white"//StudioTheme.Values.themeHoverHighlight
            }
        }
    ]
}
