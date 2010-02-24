/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

import Qt 4.6

Item {
    id: bar

    property var flickable
    property var style

    property int scrollHeight: height - handle.height

    Rectangle {
        anchors.fill: parent;
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        color: "transparent"
        border.width: 1;
        border.color: style.scrollbarBorderColor;
    }

    function moveHandle(viewportPos, updateFlickable) {
	handle.updateFlickable = updateFlickable

	if (flickable)
	    handle.y = scrollHeight * Math.min(
		viewportPos / (flickable.viewportHeight - flickable.height),
                1);
	else
	    handle.y = 0;

	handle.updateFlickable = true
    }

    function updateHandle() {
	moveHandle(flickable.viewportY, false);
    }
    
/*
    NumberAnimation {
        id: handleResetAnimation
        target: handle
        property: "y"
        from: handle.y
        to: 0
        duration: 500
    }
*/

    onFlickableChanged: moveHandle(0, true)

    Connection {
        sender: flickable
        signal: "heightChanged"
        script: moveHandle(0, true)
    }

    Connection {
        sender: flickable
        signal: "viewportHeightChanged"
        script: updateHandle()
    }

    Connection {
        sender: flickable
        signal: "positionYChanged"
        script: updateHandle()
    }

    onHeightChanged: updateHandle()
    
    MouseRegion {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: handle.top
        onClicked: {
//            handleMoveAnimation.to = Math.max(0, handle.y - 40)
//            handleMoveAnimation.start();
            handle.y = Math.max(0, handle.y - style.scrollbarClickScrollAmount)
        }
    }

    Item {
        id: handle

        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
//        anchors.rightMargin: 1
	height: Math.max(width, bar.height * Math.min(1, flickable.height / flickable.viewportHeight))

	property bool updateFlickable: true
	
	onYChanged: {
	    if (updateFlickable)
		flickable.viewportY = Math.max(0, flickable.viewportHeight * y / bar.height)
	}

        Rectangle {
            width: parent.height - 1
            height: parent.width
            y: 1 - height

            rotation: 90
            transformOrigin: Item.BottomLeft

            gradient: Gradient {
                GradientStop { position: 0.0; color: style.scrollbarGradientStartColor }
                GradientStop { position: 1.0; color: style.scrollbarGradientEndColor }
            }
        }

        MouseRegion {
            anchors.fill: parent
            drag.target: parent
            drag.axis: "YAxis"
            drag.minimumY: 0
            drag.maximumY: scrollHeight
        }
    }

    MouseRegion {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: handle.bottom
        anchors.bottom: parent.bottom
        onClicked: {
//            handleMoveAnimation.to = Math.min(scrollHeight, handle.y + 40)
//            handleMoveAnimation.start();
            handle.y = Math.min(scrollHeight, handle.y + style.scrollbarClickScrollAmount)
        }
    }
/*
    NumberAnimation {
        id: handleMoveAnimation
        target: handle
        property: "y"
        duration: 200
    }
*/
}
