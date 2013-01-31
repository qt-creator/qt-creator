/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 1.0

// scrollbar for the items view

Item {
    id: bar

    // public

    property variant flickable

    function scroll(delta) {
        handle.y = Math.max(0, Math.min(scrollHeight, handle.y + delta))
    }

    function reset() {
        handle.y = 0
    }

    // internal

    ItemsViewStyle { id: style }

    onFlickableChanged: reset()

    property int scrollHeight: height - handle.height

    clip: true

    Rectangle {
        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 3
        anchors.horizontalCenter: parent.horizontalCenter
        width: 6
        radius: 3
        color: style.scrollbarColor
        border.width: 1
        border.color: style.scrollbarBorderColor
    }

    function updateHandle() {
        handle.updateFlickable = false

        if (flickable)
            handle.y = scrollHeight * Math.min(
                    flickable.contentY / (flickable.contentHeight - flickable.height),
        1)
            else
                handle.y = 0

        handle.updateFlickable = true
    }

    onHeightChanged: updateHandle()

    Connections {
        target: flickable
        onHeightChanged: updateHandle()
    }

    Connections {
        target: flickable
        onContentHeightChanged: updateHandle()
    }

    Connections {
        target: flickable
        onContentYChanged: updateHandle()
    }

    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: handle.top
        onClicked: scroll(-style.scrollbarClickScrollAmount)
    }

    Item {
        id: handle

        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.max(width, bar.height * Math.min(1, flickable.height / flickable.contentHeight))

        property bool updateFlickable: true

        onYChanged: {
            if (updateFlickable)
                flickable.contentY = Math.max(0, flickable.contentHeight * y / bar.height)
            }
			
                Rectangle {
                    width: parent.height
                    height: parent.width
                    y: -height - 2
                    x: -2

                    rotation: 90
                    transformOrigin: Item.BottomLeft

                    border.color: style.scrollbarBorderColor
                    border.width: 1
                    radius: 3

                    gradient: Gradient {
                        GradientStop { position: 0.15; color: style.scrollbarGradientStartColor }
                        GradientStop { position: 0.78; color: style.scrollbarGradientMiddleColor }
                        GradientStop { position: 0.80; color: style.scrollbarGradientEndColor }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    drag.target: parent
                    drag.axis: "YAxis"
                    drag.minimumY: 0
                    drag.maximumY: scrollHeight
                }
            }

    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: handle.bottom
        anchors.bottom: parent.bottom
        onClicked: scroll(style.scrollbarClickScrollAmount)
    }
}
