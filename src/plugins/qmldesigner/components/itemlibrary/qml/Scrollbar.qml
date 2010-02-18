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

    property var handleBar: handle

    property var scrollFlickable
    property var style

    property bool scrolling: (scrollFlickable.viewportHeight > scrollFlickable.height)
    property int scrollHeight: height - handle.height

    Binding {
        target: scrollFlickable
        property: "viewportY"
        value: Math.max(0, scrollFlickable.viewportHeight - scrollFlickable.height) *
               handle.y / scrollHeight
    }

    Rectangle {
        anchors.fill: parent;
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        color: "transparent"
        border.width: 1;
        border.color: "#8F8F8F";
    }

    function moveHandle(viewportPos) {
        var pos;

        if (bar.scrollFlickable) {//.visibleArea.yPosition) {
            pos = bar.scrollHeight * viewportPos / (bar.scrollFlickable.viewportHeight - bar.scrollFlickable.height);
        } else
            pos = 0;

//        handleMoveAnimation.to = Math.min(bar.scrollHeight, pos)
//        handleMoveAnimation.start();
        handle.y = Math.min(bar.scrollHeight, pos)
    }

    function limitHandle() {
        // the following "if" is needed to get around NaN when starting up
        if (scrollFlickable)
            handle.y = Math.min(handle.height * scrollFlickable.visibleArea.yPosition,
                                scrollHeight);
        else
            handle.y = 0;
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
    Connection {
        sender: scrollFlickable
        signal: "heightChanged"
        script: {
            /* since binding loops prevent setting the handle properly,
               let's animate it to 0 */
            if (scrollFlickable.viewportY > (scrollFlickable.viewportHeight - scrollFlickable.height))
//                handleResetAnimation.start()
                handle.y = 0
        }
    }

    onScrollFlickableChanged: handle.y = 0

/*
    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 3
        anchors.rightMargin: 3
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        radius: width / 2
        color: style.scrollbarBackgroundColor
    }
*/
    MouseRegion {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: handle.top
        onClicked: {
//            handleMoveAnimation.to = Math.max(0, handle.y - 40)
//            handleMoveAnimation.start();
            handle.y = Math.max(0, handle.y - 40)
        }
    }

    Item {
        id: handle

        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
//        anchors.rightMargin: 1
        height: Math.max(width, bar.height * Math.min(1, scrollFlickable.visibleArea.heightRatio))

        Rectangle {
            width: parent.height - 1
            height: parent.width
            y: 1 - height

            rotation: 90
            transformOrigin: Item.BottomLeft

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#7E7E7E" }
                GradientStop { position: 1.0; color: "#C6C6C6" }
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
            handle.y = Math.min(scrollHeight, handle.y + 40)
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

